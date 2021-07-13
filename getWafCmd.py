scenario = {0: [4, 4, 10, 100000], 1: [
    9, 16, 10, 100000], 2: [16, 64, 60, 100000]}
cmd_str = './waf --run "evoMCS -runs=3 -verbose=1 -seed=%d -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -guardInterval=%d -enableObssPd=%d -useExtendedBlockAck=%d -mcs=%d -walk=0"'

indiv = [1, 5, 160, 1, 0, 1, 1, 1, 15, 0.038263513253955335]
command = cmd_str % tuple([1] + scenario[1] + indiv[0:-1])
print(command)

# best:     ./waf --run "evoMCS -runs=3 -verbose=1 -seed=1 -numAp=9 -numSta=16 -duration=10 -dataRate=100000 -technology=0 -frequency=6 -channelWidth=20 -useUdp=0 -useRts=0 -guardInterval=800 -enableObssPd=0 -useExtendedBlockAck=0 -mcs=3 -walk=0"
# worst:    ./waf --run "evoMCS -runs=3 -verbose=1 -seed=1 -numAp=9 -numSta=16 -duration=10 -dataRate=100000 -technology=1 -frequency=5 -channelWidth=160 -useUdp=1 -useRts=0 -guardInterval=1 -enableObssPd=1 -useExtendedBlockAck=1 -mcs=15 -walk=0"
