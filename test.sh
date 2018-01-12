IP=$1
REP=$2
PORT=3458
NBUFS=(10 12 15 20 30 60)
BUFSIZES=(150 125 100 75 50 25)
echo "Running Tests for Type 1"
for i in `seq 0 5`;
	do
        echo "nbufs=${NBUFS[$i]}, bufsize=${BUFSIZES[$i]}"
		for j in `seq 1 10`;
			do
				echo $(./client $PORT $REP ${NBUFS[$i]} ${BUFSIZES[$i]} $IP 1)
			done
        done

echo "Running Tests for Type 2"
for i in `seq 0 5`;
	do
        echo "nbufs=${NBUFS[$i]}, bufsize=${BUFSIZES[$i]}"
		for j in `seq 1 10`;
			do
				echo $(./client $PORT $REP ${NBUFS[$i]} ${BUFSIZES[$i]} $IP 2)
			done
        done

echo "Running Tests for Type 3"
for i in `seq 0 5`;
	do
        echo "nbufs=${NBUFS[$i]}, bufsize=${BUFSIZES[$i]}"
		for j in `seq 1 10`;
			do
				echo $(./client $PORT $REP ${NBUFS[$i]} ${BUFSIZES[$i]} $IP 3)
			done
        done

