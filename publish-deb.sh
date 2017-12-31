#! /bin/sh

# publish build packages to repository

scp $@ repo@icky:/var/web/repo.shahada.abubakar.net/apt/pool/main/
ssh repo@icky "cd /var/web/repo.shahada.abubakar.net/apt; reindex_apt.sh"
