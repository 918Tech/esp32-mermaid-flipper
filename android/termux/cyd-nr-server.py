#!/data/data/com.termux/files/usr/bin/python
"""918 Mermaid CYD NR Flasher local control service."""
from __future__ import annotations
import argparse, hashlib, json, shutil, subprocess, threading, time, uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

EXPECTED_HW="ESP32-WROOM-32E-N4"
EXPECTED_FLASH="4MB"
EXPECTED_CHIP="esp32"
EXPECTED_IDENTITY="MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1"
MAX_LOG_CHARS=24000
ROOT=Path(__file__).resolve().parent
HTML=ROOT/"cyd-nr.html"
DEFAULT_IMAGE=Path.home()/"storage"/"downloads"/"918-mermaid"/"cyd-wroom32e-n4-mermaid.factory.bin"
DEFAULT_MANIFEST=Path.home()/"storage"/"downloads"/"918-mermaid"/"cyd-wroom32e-n4-mermaid.manifest.txt"
_jobs={}
_lock=threading.Lock()
_cfg=None

def read_manifest(path: Path):
    if not path.is_file(): raise RuntimeError(f"manifest missing: {path}")
    values={}
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line=raw.strip()
        if not line or line=="MERMAID_CYD_FACTORY" or "=" not in line: continue
        key,value=line.split("=",1); values[key.strip()]=value.strip()
    if values.get("hardware")!=EXPECTED_HW: raise RuntimeError("manifest hardware does not match CYD ESP32-WROOM-32E-N4")
    if values.get("flash_size")!=EXPECTED_FLASH: raise RuntimeError("manifest flash size is not 4MB")
    sha=values.get("sha256","").lower()
    if len(sha)!=64 or any(c not in "0123456789abcdef" for c in sha): raise RuntimeError("manifest SHA-256 is invalid")
    return values

def sha256_file(path: Path):
    h=hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""): h.update(chunk)
    return h.hexdigest()

def release_status():
    data={"hardware":EXPECTED_HW,"chip":EXPECTED_CHIP,"flash_size":EXPECTED_FLASH,
          "display":"ILI9341 240x320 2.8in","touch":"XPT2046 resistive",
          "expected_identity":EXPECTED_IDENTITY,"nrflash":shutil.which("nrflash") is not None,
          "image":str(_cfg.image),"manifest":str(_cfg.manifest),"firmware_ready":False,
          "sha256":None,"error":None}
    try:
        manifest=read_manifest(_cfg.manifest)
        if not _cfg.image.is_file(): raise RuntimeError(f"factory image missing: {_cfg.image}")
        actual=sha256_file(_cfg.image); expected=manifest["sha256"].lower()
        if actual!=expected: raise RuntimeError("factory image SHA-256 mismatch")
        data["firmware_ready"]=True; data["sha256"]=actual; data["source_sha"]=manifest.get("source_sha","")
    except Exception as exc: data["error"]=str(exc)
    return data

def update_job(job_id, **changes):
    with _lock:
        job=_jobs[job_id]
        extra=changes.pop("log_append",None)
        job.update(changes)
        if extra is not None: job["log"]=(job.get("log","")+extra)[-MAX_LOG_CHARS:]

def run_job(job_id, kind):
    status=release_status()
    if not status["nrflash"]:
        update_job(job_id,state="FAILED",error="nrflash is not installed"); return
    if kind=="probe":
        args=["nrflash","probe","--chip",EXPECTED_CHIP]
    elif kind=="flash":
        if not status["firmware_ready"]:
            update_job(job_id,state="FAILED",error=status["error"] or "firmware not ready"); return
        args=["nrflash","write","--chip",EXPECTED_CHIP,"--offset","0x0",str(_cfg.image),"--verify"]
    else:
        update_job(job_id,state="FAILED",error="unsupported job"); return
    update_job(job_id,state="RUNNING",command=" ".join(args[:4])+" ...")
    try:
        proc=subprocess.Popen(args,shell=False,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,bufsize=1)
        lines=[]
        assert proc.stdout is not None
        for line in proc.stdout:
            lines.append(line); update_job(job_id,log_append=line)
        rc=proc.wait(); joined="".join(lines).lower()
        mismatch="verify mismatch" in joined
        if rc!=0 or mismatch:
            update_job(job_id,state="FAILED",returncode=rc,error="NR Flash verification mismatch" if mismatch else f"nrflash exited {rc}")
        else:
            update_job(job_id,state="SUCCEEDED",returncode=rc)
    except Exception as exc:
        update_job(job_id,state="FAILED",error=str(exc))

def create_job(kind):
    with _lock:
        if any(j.get("state") in {"QUEUED","RUNNING"} for j in _jobs.values()):
            raise RuntimeError("another NR Flash operation is already running")
        job_id=uuid.uuid4().hex[:12]
        _jobs[job_id]={"id":job_id,"kind":kind,"state":"QUEUED","created":int(time.time()),"log":"","error":None}
    threading.Thread(target=run_job,args=(job_id,kind),daemon=True).start()
    return job_id

class Handler(BaseHTTPRequestHandler):
    server_version="MermaidCYDNR/1.0"
    def log_message(self,fmt,*args): print("[http]",fmt%args)
    def send_json(self,code,payload):
        body=json.dumps(payload,separators=(",",":")).encode()
        self.send_response(code); self.send_header("Content-Type","application/json")
        self.send_header("Cache-Control","no-store"); self.send_header("Content-Length",str(len(body)))
        self.end_headers(); self.wfile.write(body)
    def do_GET(self):
        path=urlparse(self.path).path
        if path in {"/","/cyd-nr.html"}:
            if not HTML.is_file(): self.send_error(404); return
            body=HTML.read_bytes(); self.send_response(200)
            self.send_header("Content-Type","text/html; charset=utf-8"); self.send_header("Cache-Control","no-store")
            self.send_header("Content-Length",str(len(body))); self.end_headers(); self.wfile.write(body); return
        if path=="/api/status": self.send_json(200,release_status()); return
        if path.startswith("/api/jobs/"):
            job_id=path.rsplit("/",1)[-1]
            with _lock: job=dict(_jobs.get(job_id,{}))
            self.send_json(200,job) if job else self.send_json(404,{"error":"job not found"}); return
        self.send_error(404)
    def do_POST(self):
        path=urlparse(self.path).path
        if path not in {"/api/probe","/api/flash"}: self.send_error(404); return
        try: self.send_json(202,{"job_id":create_job("probe" if path.endswith("probe") else "flash")})
        except RuntimeError as exc: self.send_json(409,{"error":str(exc)})

def main():
    global _cfg
    p=argparse.ArgumentParser()
    p.add_argument("--image",type=Path,default=DEFAULT_IMAGE)
    p.add_argument("--manifest",type=Path,default=DEFAULT_MANIFEST)
    p.add_argument("--port",type=int,default=9180)
    _cfg=p.parse_args()
    print("918 MERMAID CYD NR FLASHER")
    print(f"Target: {EXPECTED_HW} / {EXPECTED_FLASH} / XPT2046 resistive touch")
    print(f"UI: http://127.0.0.1:{_cfg.port}")
    ThreadingHTTPServer(("127.0.0.1",_cfg.port),Handler).serve_forever()
if __name__=="__main__": main()
