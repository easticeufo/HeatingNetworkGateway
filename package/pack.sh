#!/bin/bash

DATE=$(date +%Y%m%d)

#��Ҫ�����firmware���ļ�
PACK_FILE="HeatingNetworkGateway appweb.conf initrun.sh shell_cmd webfile.tar"

./pack firmware${DATE} ${PACK_FILE}
