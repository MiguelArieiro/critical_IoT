# cenario_IoT
# configuring and building ns-3 (with examples and tests)
```
cd /ns-allinone-3.33/ns-3.33
```
```
./waf clean
./waf configure --build-profile=debug --enable-examples --enable-tests
./waf
```

# running file
place critical_iot.cc file on the /ns-allinone-3.33/ns-3.33/scratch
```
./waf --run critical_iot
```
# running file with arguments
```
./waf --run "critical_iot --PrintHelp"
```

e.r. number of AP=3 and numebr of stations/ap = 5
```
./waf --run "critical_iot --numAp=3 --numSta=5"
```
