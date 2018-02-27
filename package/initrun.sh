ifconfig lo up
sleep 1

udhcpc -x hostname:HeatingNetworkGateway &
sleep 5

export TZ=CST-8

WORK_DIR="/opt"

cp HeatingNetworkGateway ${WORK_DIR}
cp appweb.conf ${WORK_DIR}
cp shell_cmd ${WORK_DIR}
rm -rf ${WORK_DIR}/webfile
tar -xf webfile.tar -C ${WORK_DIR}

cd ${WORK_DIR}

ln -s $(pwd)/shell_cmd /bin/prtHardInfo
ln -s $(pwd)/shell_cmd /bin/setDebugLevel
ln -s $(pwd)/shell_cmd /bin/xbeeAT
ln -s $(pwd)/shell_cmd /bin/prtXbeeFrameOpen
ln -s $(pwd)/shell_cmd /bin/prtXbeeFrameClose
ln -s $(pwd)/shell_cmd /bin/prtMqttMessage

echo "run HeatingNetworkGateway!"
./HeatingNetworkGateway &
