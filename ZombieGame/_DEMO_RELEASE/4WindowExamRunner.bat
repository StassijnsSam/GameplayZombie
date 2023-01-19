start GPP_TEST_RELEASE.exe -x 30 -y 30 -s 9

start GPP_TEST_RELEASE.exe -x 1000 -y 30 -s 10

start GPP_TEST_RELEASE.exe -x 30 -y 540 -s 11

start GPP_TEST_RELEASE.exe -x 1000 -y 540 -s 12
echo new ActiveXObject("WScript.Shell").AppActivate("GPP_TEST_RELEASE.exe"); > tmp.js
cscript //nologo tmp.js & del tmp.js