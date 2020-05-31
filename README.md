# cenario_IoT
 

# configuring and building ns-3 (with examples and tests)
```
cd /ns-allinone-3.30/ns-3.30
```
```
./waf clean
./waf configure --build-profile=debug --enable-examples --enable-tests
./waf
```

# running file

place wifi-spatial-reuse-var.cc file on the /ns-allinone-3.30/ns-3.30/scratch
```
./waf --run wifi-spatial-reuse-var
```
# running file with arguments
```
./waf --run "wifi-spatial-reuse-var --PrintHelp"
```

# e.r. number of AP=3 and numebr of stations/ap = 5
```
./waf --run "wifi-spatial-reuse-var --numAp=3 --numSta=5"
```
