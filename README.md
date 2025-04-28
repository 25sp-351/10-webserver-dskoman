## 10-webserver ##

Web server with three "functions":

/calc/add/<num1>/<num2>
/calc/mul/<num1>/<num2>
/calc/div/<num1>/<num2>

## Prerequisites ##

- gcc or an equivalent C compiler is installed
- Make is installed
- telnet is installed

## Building with Make ##

1. Build with `make all`
2. The executable created is named `main.out`

## Running the server ##

`./main.out` starts the server and listens on port 80
`./main.out <port>` starts the server and listens on <port>

## Testing with telnet ##

1. Start a new terminal separate from the listening server
2. While the server is running, in the new terminal run the command `telnet <listening port>`
3. After successfully connecting, interact with the server with any of the following GET requests:

- GET `/calc/add/<num1>/<num2>`
- GET `/calc/mul/<num1>/<num2>`
- GET `/calc/div/<num1>/<num2>`