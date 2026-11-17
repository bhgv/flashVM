# Copyright (c) 2026 bhgv
# EN: Provision a persistent installation ID and editable nickname in an explicitly selected file.
# RU: Создать постоянный ID установки и изменяемый nickname в явно выбранном файле.
import argparse
import base64
import json
import os
from pathlib import Path
import secrets
import tempfile


# EN/RU: Validate copied metadata; identifiers describe an installation, never authenticate a peer.
# Return the original mapping or raise ValueError; no file mutation.
# Проверить metadata без изменения файла; вернуть mapping либо ValueError, ID не credentials.
def validate(data):
    if not isinstance(data, dict) or set(data) != {'version', 'systemid', 'nickname'} or data['version'] != 1:
        raise ValueError('invalid installation metadata')
    identity, nickname = data['systemid'], data['nickname']
    if not isinstance(identity, str) or len(identity) != 43:
        raise ValueError('invalid system ID')
    try:
        raw = base64.b64decode(identity+'=', altchars=b'-_', validate=True)
    except ValueError as error:
        raise ValueError('invalid system ID') from error
    if len(raw) != 32 or base64.urlsafe_b64encode(raw).decode().rstrip('=') != identity:
        raise ValueError('noncanonical system ID')
    if not isinstance(nickname, str) or len(nickname.encode('utf-8')) >= 128 or any(ord(c) < 32 for c in nickname):
        raise ValueError('invalid nickname')
    return data


# EN/RU: Publish complete JSON atomically. Exclusive creation loses safely to another creator.
# Return1 if published,0 on an existing creation target; caller owns data/path.
# Атомарно опубликовать JSON; exclusive create возвращает0 при существующем target, иначе1.
def publish(path, data, exclusive):
    descriptor, temporary = tempfile.mkstemp(prefix='system-id-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w', encoding='utf-8') as output:
            json.dump(data, output, ensure_ascii=False, sort_keys=True)
            output.write('\n')
            output.flush()
            os.fsync(output.fileno())
        if exclusive:
            try:
                os.link(temporary, path)
            except FileExistsError:
                return 0
        else:
            os.replace(temporary, path)
        return 1
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


# EN/RU: Create/read explicit path, optionally update nickname while retaining systemid.
# No hardware fingerprint, network lookup or implicit home-directory storage.
# Создать/прочитать explicit path; optional nickname update сохраняет systemid, без hardware/network.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--file', type=Path, required=True)
    parser.add_argument('--nickname')
    args = parser.parse_args()
    path = args.file
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.is_symlink():
            raise ValueError('identity file must not be a symbolic link')
        initial = validate({'version': 1, 'systemid': base64.urlsafe_b64encode(secrets.token_bytes(32)).decode().rstrip('='),
                            'nickname': args.nickname or ''})
        if not path.exists():
            publish(path, initial, True)
        data = validate(json.loads(path.read_text(encoding='utf-8')))
        if args.nickname is not None and data['nickname'] != args.nickname:
            data['nickname'] = args.nickname
            validate(data)
            publish(path, data, False)
        print(json.dumps(data, ensure_ascii=False, sort_keys=True))
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
