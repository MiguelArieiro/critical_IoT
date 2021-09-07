
import os
import subprocess as sp
import csv
import time

waf = "../waf"
dir_res = "xres_v3/"
dir_exp = "e"

# Read settings
with open('Metricsv3.csv', encoding='UTF-8') as csvfile:
	aux = csv.reader(csvfile, delimiter=';')
	next(aux) # ignore header
	for row in aux:
		#print(" ".join(row))
		bie = row[0].encode('utf-8').strip() #["ExpID"]
		bcmd_exec = row[27].encode('utf-8').strip() #["cmd"]
		status = row[2].encode('utf-8').strip() #["Status"]
		ie = bie.decode('utf-8')
		cmd_exec = bcmd_exec.decode('utf-8')
		s_status = status.decode('utf-8')

		#print(ie)
		#print(bie)
		#aie = int(ie)
		if (len(s_status)>0):
			print("ignoring" + str(ie) + "with " + s_status )
			continue
		print("Running " + str(ie))

#for ie in ('01', '02', '03', '04'):
		# create dir for results
		dirExp = dir_res + dir_exp + ie
		idExp = dir_exp + ie

		if (not os.path.exists(dirExp)):
			print("to create dir " + dirExp)
			os.mkdir(dirExp)

		#run the command
		#cmd_exec = "./waf --run \"critical_MCS --duration=10 --dataRate=60000 --technology=0 --numAp=1 --numSta=2 --enableObssPd=1 --powSta=18 --powAp=23 --ccaEdTrSta=-62 --ccaEdTrAp=-62 --rxSensSta=-54 --rxSensAp=-99 --mcs=11 --udp=1 --batteryLevel=200 --distance=20--TCPpayloadSize=150 --UDPpayloadSize=64 --frequency=2 --channelWidth=20 --numTxSpatialStreams=1 --numRxSpatialStreams=1 --numAntennas=4 --voice=true \""
		#cmd_exec = "time " + cmd_exec
		print(cmd_exec)
		log_file = "alogExp_" +idExp
		with open(log_file, "w") as outfile:
			start = time.time()
			sp.run(cmd_exec, shell=True, check=True, stdout=outfile)
			elpased = time.time() - start
			print("Time:" + str(elpased))


		#Parse Flow results
		cmd_exec = "python runSimulationParser.py testflow.xml "
		sp.run(cmd_exec, shell=True, check=True)

		#put results in the dir
		if (os.path.exists(dirExp)):
			cmd_exec = "mv FI* testflow.xml alogExp_e* flow_output.txt " + dirExp + "/"
			print(cmd_exec)
			sp.run(cmd_exec, shell=True, check=True)
