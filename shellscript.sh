./waf --run "scratch/SendPacket --numNodes=5 --message=101 --option=1"
./waf --run "scratch/SendPacket --numNodes=3 --message=10111 --option=2"
export NS_LOG=WifiSimpleAdhocGrid=level_info
./waf --run "scratch/SendPacket --numNodes=3"
