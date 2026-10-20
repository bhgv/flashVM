#!/usr/bin/env python3
"""Child image loads reuse ordinary owner delivery, identity, unloading and SWF reload."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import base64
import os
from pathlib import Path
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import zlib
from child_loading import push, global_call, method_call, swf, require
from async_loading import get, when, equal
from unload_loading import require_undefined

JPEG = base64.b64decode('/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAAIBAQEBAQIBAQECAgICAgQDAgICAgUEBAMEBgUGBgYFBgYGBwkIBgcJBwYGCAsICQoKCgoKBggLDAsKDAkKCgr/2wBDAQICAgICAgUDAwUKBwYHCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgr/wAARCAAIAAgDASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD8n6KKK+LP0Q//2Q==')
GIF = bytes.fromhex('47494638396101000100800000000000ff000021f90401000000002c00000000010001000002024401003b')


def png():
    def chunk(kind, data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    row = b'\0' + bytes([64,32,16,128])*8
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',8,8,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(row*8))+chunk(b'IEND',b'')


def content_query(owner, method):
    """Invoke a zero-argument MovieClip query through ordinary ActionCallMethod."""
    return push(0,owner)+b'\x1c'+push(method)+b'\x52'


def require_content(owner, method, expected):
    """Require exact completed byte/version metadata in an actual SWF frame."""
    failed=push('content query mismatch: '+method)+b'\x2a'
    return content_query(owner,method)+push(expected)+b'\x66\x9d\x02\0'+struct.pack('<h',len(failed))+failed


def dimension(index):
    return push('_root.picture', index) + b'\x22'


def named_dimension(index):
    """GetMember uses the same intrinsic geometry as numeric ActionGetProperty."""
    return get('_root.picture') + push('_width' if index == 8 else '_height') + b'\x4e'


def require_named_dimension(index, expected):
    """Require an exact dimension read through the ordinary property descriptor."""
    failed = push('named dimension mismatch') + b'\x2a'
    return named_dimension(index) + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h',len(failed)) + failed


def assign_member(name, value):
    """Assign through SetMember, retaining an explicit MovieClip receiver."""
    return get('_root.picture') + push(name,value) + b'\x4f'


def require_dimension(index, expected):
    failed = push('image dimension mismatch') + b'\x2a'
    return dimension(index) + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h',len(failed)) + failed


def root(kind=0, expected_source=None):
    width = 1 if kind == 1 else 8
    payload_size = len([png(),GIF,JPEG][kind])
    first = method_call('_root','createEmptyMovieClip','picture',1)
    first += method_call('_root.picture','createEmptyMovieClip','old',1)
    first += push('_root.picture.kept',42)+b'\x1d'+push('_root.saved')+get('_root.picture')+b'\x1d'
    first += push('_root.iter',0)+b'\x1d'+push('_root.phase',0)+b'\x1d'
    first += require_content('_root','getSWFVersion',8)
    first += get('_root.picture')+push('image.bin',2,'loadMovie')+b'\x3d\x17'
    ready = require('_root.picture.kept',42)+require_dimension(9,width)
    ready += require_content('_root.picture','getBytesLoaded',payload_size)
    ready += require_content('_root.picture','getBytesTotal',payload_size)
    ready += require_content('_root.picture','getSWFVersion',-1)
    ready += require_named_dimension(8,width) + require_named_dimension(9,width)
    ready += assign_member('_width',width*2) + assign_member('_height',width*3)
    ready += require_dimension(8,width*2) + require_dimension(9,width*3)
    ready += require_named_dimension(8,width*2) + require_named_dimension(9,width*3)
    ready += push('_root.imageOrigin')+get('_root.picture')+push('_url')+b'\x4e\x1d'
    ready += assign_member('_url','ignored source')
    failed_url = push('readonly image origin changed')+b'\x2a'
    ready += get('_root.imageOrigin')+get('_root.picture')+push('_url')+b'\x4e\x66\x9d\x02\0'+struct.pack('<h',len(failed_url))+failed_url
    ready += require_undefined('_root.picture.old')
    failed=push('image target changed')+b'\x2a'
    ready += get('_root.saved')+get('_root.picture')+b'\x66\x9d\x02\0'+struct.pack('<h',len(failed))+failed
    if expected_source is not None:
        ready += require('_root.picture._url',expected_source)
    ready += push('_root.phase',1)+b'\x1d'+method_call('_root.picture','unloadMovie')
    ready += global_call('loadMovie','after.swf','picture')
    # Queued unload/reload has not published new content during this same source frame.
    ready += require_content('_root.picture','getBytesTotal',payload_size)
    second = push('_root.iter')+get('_root.iter')+push(1)+b'\x47\x1d'
    second += when(get('_root.iter')+push(200)+b'\x67',require('_root.finished',1))
    second += when(equal('_root.phase',0)+(dimension(8)+push(width)+b'\x66')+b'\x10',ready)
    # The replacement SWF has run its own frame and therefore owns the published record.
    completed = require('_root.picture.kept', 42)
    completed += require_dimension(8, 0) + require_named_dimension(8, 0)
    completed += require_content('_root.picture', 'getSWFVersion', 8)
    completed += require_content('_root.picture', 'getBytesLoaded', len(after()))
    completed += require_content('_root.picture', 'getBytesTotal', len(after()))
    completed += push('_root.finished', 1) + b'\x1d\x07'
    second += when(equal('_root.picture.loaded', 1), completed)
    return swf([first,second,b'\x81\x02\0\x01\0\x06'])


def after():
    return swf([push('this')+b'\x1c'+push('loaded',1)+b'\x4f\x07'])


class Handler(BaseHTTPRequestHandler):
    paths=[]
    kind=0
    bad=False
    redirect=False
    def log_message(self,*_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        scheme='https' if isinstance(self.connection,ssl.SSLSocket) else 'http'
        origin=scheme+'://'+self.headers['Host']+('/assets/final.bin' if self.redirect else '/app/image.bin')
        if self.redirect and self.path=='/app/image.bin':
            self.send_response(302);self.send_header('Location','/assets/final.bin');self.send_header('Content-Length','0');self.end_headers();return
        resources={'/app/start.swf':root(self.kind,origin),'/app/image.bin': [png(),GIF,JPEG][self.kind],'/app/after.swf':after()}
        resources['/assets/final.bin']=resources['/app/image.bin']
        if self.path not in resources:
            self.send_error(404);return
        data=resources[self.path]
        if self.bad and self.path.endswith('image.bin'):
            data=data[:len(data)//2]
        self.send_response(200);self.send_header('Content-Type','application/octet-stream')
        self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)


def main():
    player,mode,*tls=sys.argv[1:];player=str(Path(player).resolve())
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software')
    command=[player,mode,'--loads','8','--frames','500','--fps','1000','--mute']
    with tempfile.TemporaryDirectory() as directory:
        folder=Path(directory);(folder/'after.swf').write_bytes(after())
        for kind,data in enumerate([png(),GIF,JPEG]):
            (folder/'start.swf').write_bytes(root(kind,str(folder/'image.bin')));(folder/'image.bin').write_bytes(data)
            result=subprocess.run(command+[str(folder/'start.swf')],env=env,capture_output=True,timeout=10)
            if mode=='--jit' and b'requested JIT backend unavailable' in result.stderr:raise SystemExit(77)
            assert result.returncode==0,result.stderr
    if b'background_provider=no' in subprocess.check_output([player,'--version']):return
    certificates_folder=tempfile.TemporaryDirectory() if tls else None
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler);extra=[]
    if tls:
        from socket_tls import certificates
        folder=Path(certificates_folder.name);ca,key=certificates(folder,tls[0])
        context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(str(folder/'valid.pem'),str(key))
        server.socket=context.wrap_socket(server.socket,server_side=True);extra=['--http-ca-file',str(ca)]
    threading.Thread(target=server.serve_forever,daemon=True).start()
    try:
        for kind in range(3):
            for asynchronous in (False,True):
                Handler.paths.clear();Handler.kind=kind
                policy=['--async-loads' if asynchronous else '--background-loads']
                result=subprocess.run(command+policy+extra+[('https' if tls else 'http')+'://127.0.0.1:%d/app/start.swf'%server.server_port],env=env,capture_output=True,timeout=10)
                assert result.returncode==0,result.stderr
                assert Handler.paths==['/app/start.swf','/app/image.bin','/app/after.swf'],Handler.paths
        # The image records its final URL; the root caller still resolves its own reload URL.
        Handler.redirect=True;Handler.kind=0
        for asynchronous in (False,True):
            Handler.paths.clear()
            policy=['--async-loads' if asynchronous else '--background-loads','--redirects','1']
            result=subprocess.run(command+policy+extra+[('https' if tls else 'http')+'://127.0.0.1:%d/app/start.swf'%server.server_port],env=env,capture_output=True,timeout=10)
            assert result.returncode==0,result.stderr
            assert Handler.paths==['/app/start.swf','/app/image.bin','/assets/final.bin','/app/after.swf'],Handler.paths
        Handler.redirect=False
        Handler.bad=True
        result=subprocess.run(command+['--async-loads']+extra+[('https' if tls else 'http')+'://127.0.0.1:%d/app/start.swf'%server.server_port],env=env,capture_output=True,timeout=10)
        assert result.returncode==1,result.stderr
    finally:
        server.shutdown();server.server_close()
        if certificates_folder:certificates_folder.cleanup()
    print('PASS PNG/GIF/JPEG local/waiting/async, opaque MIME, unload/SWF reload and malformed response')


if __name__=='__main__':main()
