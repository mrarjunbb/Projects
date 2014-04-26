grep -c 'Debug : Inside dcnet send public key index1=' log.txt
grep 'Public key counter' log.txt | head -1

echo "********"
grep -c 'receivepublickeycallback' log.txt
