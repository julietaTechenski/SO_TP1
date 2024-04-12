# # TP1 - Sistemas Operativos
# 20241Q

### Environment setup

The following packages must be installed:

    gcc make

### Docker setup

Execute the following commands to install Docker:
    
    sudo apt install docker.io

    sudo groupadd docker

    sudo usermod -aG docker $USER 

Once installed, download the image necessary:

    docker pull agodio/itba-so:2.0

On the project directory, start the container:

    docker run -v ${PWD}:/root --privileged -ti --name SO agodio/itba-so:2.0

    docker exec -ti SO bash

### Project compilation

Execute the following commands on the project directory:

    make clean all

### Project execution

There needs to be at least a file in order to execute the project. File or files must be inside docker container. We will be calling it FILES_PATH.
From the project directory run:

    ./app FILES_PATH | ./view
    
Another way to execute the projecto would be to open two terminals inside the docker container and run:

Terminal 1:

    ./app FILES_PATH

Terminal 2:

    ./view /app_shm

Last option would be to execute app.c in foreground and view.c in backround:

Terminal 1:

    ./app FILES_PATH

Terminal 2:

    ./view /app_shm &

### Output

The output of the project should be found printed in the terminal where view was ran and on the file "md5_output.txt".

## Authors:
              Campoli, Lucas - 63295
              Taurian, Magdalena - 62828
              Techenski, Julieta - 62547
