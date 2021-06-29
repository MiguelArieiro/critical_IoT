scenario = {0: [4, 4, 10, 100000], 1: [
    9, 16, 10, 100000], 2: [16, 64, 60, 100000]}
cmd_str = './waf --run "evoMCS -runs=3 -walk=0 -verbose=1 -seed=%d -numAp=%d -numSta=%d -duration=%d -dataRate=%d -technology=%d -frequency=%d -channelWidth=%d -useUdp=%d -useRts=%d -guardInterval=%d -enableObssPd=%d -useExtendedBlockAck=%d -mcs=%d"'

indiv = [0, 6, 20, 0, 0, 800, 0, 0, 3, 4.89346169461882e-06]

command = cmd_str % tuple([1] + scenario[1] + indiv[0:-1])
print(command)
