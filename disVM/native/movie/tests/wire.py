# Copyright (c) 2026 bhgv
# EN: Execute real nested movies over original libstyx TCP with conflicting terminal/provider files.
# RU: Выполнить nested movies через original libstyx TCP с конфликтующими terminal/provider files.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile


# EN/RU: Read explicit SDK raw oracle bytes, preserving them independently of tested encoders.
# Прочитать raw oracle bytes явно выбранного SDK без encoder проверяемого протокола.
def fixture(path, name):
    text = path.read_text()
    body = re.search(r'\b'+name+r'\[\]\s*=\s*\{(.*?)\};', text, re.S).group(1)
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    return bytes(int(x.strip(), 0) for x in body.split(',') if x.strip())


# EN/RU: Create explicit conflicting grants, run one TCP owner and independent player, retire both.
# Создать conflicting grants, выполнить TCP owner/player и завершить оба процесса.
def scenario(server, player, sdk, mode, deny, fault=None, sink=False, fragment=False, announced=None):
    with tempfile.TemporaryDirectory(prefix='styx movie ') as directory:
        work = Path(directory)
        terminal = work/'terminal'
        provider = work/'provider'
        prefix = '/'.join(['abcdefgh'*8]*5)+'/' if fragment else ''
        for root in (terminal, provider):
            (root/prefix/'nested').mkdir(parents=True)
        movies = sdk/'frontend/tests/managed_movies.h'
        root_movie = fixture(movies, 'managed_root_movie')
        child_movie = fixture(movies, 'managed_child_movie')
        simple = fixture(sdk/'examples/player/embedded_movie/movie.h', 'embedded_movie')
        (provider/prefix/'root.swf').write_bytes(root_movie)
        (provider/prefix/'nested/child.swf').write_bytes(child_movie)
        (terminal/prefix/'root.swf').write_bytes(b'invalid terminal code')
        (terminal/prefix/'nested/child.swf').write_bytes(b'invalid terminal code')
        for root, suffix in ((terminal, ''), (provider, '-wrong')):
            (root/prefix/'root-vars.swf').write_text('greeting=hello+from+root'+suffix)
            (root/prefix/'nested/child-vars.txt').write_text('answer=child+origin+works'+suffix)
            (root/'denied.swf').write_bytes(simple)
            (root/'partial.swf').write_bytes(simple)
            (root/'simple.swf').write_bytes(simple)
        (work/'root.swf').write_bytes(simple)
        (work/'denied.swf').write_bytes(simple)
        (work/'partial.swf').write_bytes(simple)
        (provider/'escape.swf').symlink_to(work/'denied.swf')
        name = 'simple.swf' if fault == 'noend' else deny if isinstance(deny, str) else 'denied.swf' if deny else prefix+'root.swf'
        environment = dict(os.environ)
        if fault:
            environment['MOVIE_TEST_FAULT'] = fault
        if announced:
            environment['MOVIE_TEST_ANNOUNCE'] = announced
        with (work/'server.log').open('w+') as log:
            process = subprocess.Popen([server, terminal, provider], stdout=subprocess.PIPE,
                                       stderr=log, text=True, cwd=work,
                                       env=environment)
            try:
                port = process.stdout.readline().strip()
                if not port.isdecimal():
                    raise RuntimeError('server startup '+port)
                env = dict(environment, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
                run = subprocess.run([player, port, mode, name,
                                      'deny' if deny or (fault and fault != 'coalesce') else 'sink' if sink else 'allow'], cwd=work, env=env,
                                     capture_output=True, text=True, timeout=30)
                process.wait(timeout=10)
                log.seek(0)
                output = log.read()
                if run.returncode or process.returncode or 'PASS real TCP Styx S-record managed movie' not in run.stdout:
                    raise RuntimeError(str((run.returncode, process.returncode))+run.stdout+run.stderr+output+process.stdout.read())
                if not deny and (not fault or fault == 'coalesce') and announced not in ('crc', 'version'):
                    for line in ('OPEN P '+prefix+'root.swf 0', 'OPEN P '+prefix+'nested/child.swf 0',
                                 'OPEN T '+prefix+'root-vars.swf 0', 'OPEN T '+prefix+'nested/child-vars.txt 0'):
                        if line not in output:
                            raise RuntimeError('Missing actual VFS selection '+line+'\n'+output)
                    if 'OPEN T '+prefix+'root.swf' in output or 'OPEN P '+prefix+'root-vars.swf' in output:
                        raise RuntimeError('Wrong code/data intent order '+output)
                    if mode == 'jit' and not re.search(r'native=[1-9][0-9]*', run.stderr):
                        raise RuntimeError('No actual JIT execution '+run.stderr)
                if announced:
                    remaining = process.stdout.read()
                    if announced not in ('crc', 'version') and 'WALK movie/1' not in remaining:
                        raise RuntimeError('No walk to advertised root '+remaining)
                    if announced in ('normal', 'changing') and 'WALK movie/2' not in remaining:
                        raise RuntimeError('No new generation walk '+remaining)
                if deny == 'partial.swf' and 'PARTIAL 44' not in output:
                    raise RuntimeError('Missing acknowledged full SWF failure '+output)
                print('PASS TCP Styx/S-record nested SWF '+mode+(' announced '+announced if announced else ' '+fault if fault else ' fragmented' if fragment else ' sink refusal/reuse' if sink else ' denied '+name if deny else ' code/data union'), flush=True)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()


# EN/RU: Exercise interpreter/JIT and denied initial code with a valid native fallback file present.
# Проверить interpreter/JIT и отказ code при существующем валидном local fallback file.
def main():
    server, player, sdk = (Path(x).resolve() for x in sys.argv[1:4])
    for mode in ('interpreter', 'jit'):
        for deny in (False, True):
            scenario(server, player, sdk, mode, deny)
        scenario(server, player, sdk, mode, 'partial.swf')
        scenario(server, player, sdk, mode, 'escape.swf')
        scenario(server, player, sdk, mode, False, fragment=True)
        scenario(server, player, sdk, mode, False, sink=True)
        for announced in ('normal', 'changing', 'eof', 'disconnect', 'crc', 'version'):
            scenario(server, player, sdk, mode, False, announced=announced)
        for fault in ('crc', 'eof', 'truncate', 'noend', 'coalesce'):
            scenario(server, player, sdk, mode, False, fault=fault)


if __name__ == '__main__':
    main()
