# Reproducible source archives / Воспроизводимые архивы исходников

`scripts/source_archive.py` exports committed sources and the complete Git ancestry
reachable from a selected commit. It creates an independent repository with master
at that commit and origin pointing to the authorized mirror. No remote write is
performed by the script. Uncommitted files, unrelated refs, private stashes,
reflogs, host configuration, build output and worktree links are not copied.

Requires Python 3.8+ and Git 2.29+ with the repository's object-format support:

```sh
python scripts/source_archive.py --output /absolute/path/FlashVM-source-v0.247.zip
```

The versioned filename must match committed VERSION.txt, so run after committing.
Use --repo and --revision to select an explicit repository/commit. Existing output
files and shallow histories are rejected. The script never modifies the source
repository; temporary staging data is created beside the destination and removed.

Tracked contents, executable modes and Git objects are exported from the selected
commit. ZIP timestamps derive from its commit time; entries are sorted and the
index is rebuilt without checkout-time stat data. Single-thread deterministic Git
packing avoids existing local pack layout/delta decisions. Git fsck, ancestry count
and ZIP CRC checks run before the output is made available. SHA256 is printed.

Two fresh exports on the current Linux/Git/Python toolchain were byte-identical.
An extracted copy passed full strict fsck, matched the selected HEAD and all 340
reachable commits, and had a clean Git worktree. Cross-version Git/zlib/Python byte
identity and execution on other OSes are not established by these checks. This is
one release-packaging gate; it does not close roadmap stage 9 or earlier stages.

Скрипт создаёт архив только зафиксированных исходников и всей достижимой истории
выбранного коммита. В архиве независимый Git-репозиторий с origin зеркала; локальные
пути worktree, частные stash, reflog и результаты сборки не копируются. Метаданные
ZIP и порядок файлов фиксированы, индекс строится без времён локального checkout.
Проверяются целостность Git, число коммитов и CRC архива; существующий файл не
перезаписывается. Два экспорта в текущей Linux-среде совпали побайтно, распакованная
копия прошла fsck и проверку чистого состояния. Совпадение между разными версиями
инструментов и приёмка Windows/macOS/Android пока не подтверждены.
