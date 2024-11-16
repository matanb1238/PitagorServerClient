int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind the socket to the IP and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    
    // Main server loop: Accept and handle each client
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        
        printf("New client connected: %d\n", client_fd);
        
        // Handle the client request
        handle_client(client_fd);
    }
    
    // Close the server socket (this point is never reached in this loop)
    close(server_fd);
    
    return 0;
}
