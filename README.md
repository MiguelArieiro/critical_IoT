# EVOMCS

## Setup

### EvoMCS.py
```
sudo apt install python3 python3-pip
pip3 install numpy 
```

## EvoMCS.cc and ns-3 
```
sudo apt install g++ python3 python3-dev pkg-config sqlite sqlite3 libsqlite3-dev python3-setuptools git libxml2 libxml2-dev
```
### ns-3
```
git clone https://gitlab.com/nsnam/ns-3-allinone.git
cd ns-3-allinone/
./download.py -n ns-3.33
cd ns-3.33
./waf clean
./waf configure --build-profile=optimized --enable-examples --enable-tests
./waf
```

## Running evolutionary algorithm - EvoMCS.py
Place evoMCS.cc in /ns-allinone-3.33/ns-3.33/scratch.
Place evoMCS.py in /ns-allinone-3.33/ns-3.33/

```
sudo python3 evoMCS.py
```

## Running simulation scenario - EvoMCS.cc
Place evoMCS.cc in /ns-allinone-3.33/ns-3.33/scratch

```
./waf --run evoMCS
```
### Arguments
To get a full list of avaialable arguments:
```
./waf --run "evoMCS --PrintHelp"
```

e.r. number of AP=3 and numebr of stations/ap = 5
```
./waf --run "evoMCS --numAp=3 --numSta=5"
```
