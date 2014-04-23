./waf --run scratch/SendPacket --command-template="valgrind --verbose --tool=memcheck --show-reachable=yes --track-fds=yes --trace-children=yes --track-origins=yes --leak-check=full --log-file=valgrind.log %s"

