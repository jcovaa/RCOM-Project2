# RC - Project 2

## FTP Download Application

This application implements a simple FTP client that supports URLs in the format:

    ftp://[<user>:<password>@]<host>/<url-path>

If no user credentials are provided, the app uses anonymous FTP authentication, with both username and password set to "anonymous".

### Compilation
```
gcc -Wall download.c -o download
```

### Execution
```
./download ftp://mirrors.up.pt/debian/README.html
```

## Students
- Diogo Lima da Silva Costa, up202108655
- João Rodrigues Vila Cova, up202307756
