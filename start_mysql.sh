printf "\n"
printf "Starting mysql service..."
printf "\n"
sudo /etc/init.d/mysql start
printf "\n"
printf "mysql should be running as of now"
( trap exit SIGINT ; read -r -d '' _ </dev/tty ) ## wait for Ctrl-C
printf "\n"
printf "control + c detected, exiting."
printf "\n"
sudo /etc/init.d/mysql stop
