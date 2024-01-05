#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PAYLOAD_SIZE 2048
#define MAX_CONNECTIONS 5
#define MAX_ADDR_BUFFER 128
#define MAX_HASHMAP_ENTRIES 4096
#define MAX_HASHMAP_KEY_SIZE 256
#define MAX_HASHMAP_VALUE_SIZE 2048

// Hashmap implementation
// As simple as it gets
// Doesn't handle collisions
typedef struct {
    char key[MAX_HASHMAP_KEY_SIZE];
    char value[MAX_HASHMAP_VALUE_SIZE];
} hashmap_entry;

typedef struct {
    hashmap_entry entries[MAX_HASHMAP_ENTRIES];
    int entries_count;
} hashmap;

hashmap *init_hashmap() {
    hashmap *map = malloc(sizeof(hashmap));
    map->entries_count = 0;
    return map;
}

int hash(char *key) {
    int hash = 0;
    for (int i = 0; i < strlen(key); i++) {
        hash += key[i];
    }

    hash = hash % MAX_HASHMAP_ENTRIES;

    return hash;
}

int hashmap_put(hashmap *map, char *key, char *value) {
    int h = hash(key);

    if (map->entries_count >= MAX_HASHMAP_ENTRIES) {
        perror("Hashmap is full");
        return 1;
    }

    if (strlen(key) >= MAX_HASHMAP_KEY_SIZE) {
        perror("Key is too long");
        return 1;
    }

    if (strlen(value) >= MAX_HASHMAP_VALUE_SIZE) {
        perror("Value is too long");
        return 1;
    }

    if (!strcmp(map->entries[h].key, "")) {
        perror("Hashmap collision");
        return 1;
    }

    strcpy(map->entries[h].key, key);
    strcpy(map->entries[h].value, value);
    map->entries_count++;

    return 0;
}

// hashmap_get returns the value for the given key by hashing the key
// and returning the value at that index.
//
// If the key doesn't exist then it returns NULL
char *hashmap_get(hashmap *map, char *key) {
    int h = hash(key);
    for (int i = 0; i < map->entries_count; i++) {
        if (hash(map->entries[i].key) == h) {
            return map->entries[i].value;
        }
    }
    return NULL;
}

/* The way a TCP server works:
 1. Create a server socket
 2. Bind that socket to an address
 3. Listen to server socket
 4. Accept connections (This is a blocking call, as it will wait infinitely for
 any new connection)
 4.1. Once a connection is accepted, create a new thread to
 handle that connection
 4.2. The new thread will handle the connection and close
 it once it's done
 4.2.1 The new thread will also handle the three way handshake
 4.2.2 The new thread will also handle the data transfer
 4.2.3 The new thread will also handle the three way handshake
 4.2.4 The new thread will also handle the closing of the connection
 5. Close the server socket
 */

/* Three way handshake:
 * 1. Client sends a SYN packet to the server
 * 2. Server sends a SYN ACK packet to the client
 * 3. Client sends an ACK packet to the server
 *
 * A SYN packet has the SYN flag set to 1.
 * A SYN ACK packet has the SYN and ACK flags set to 1 and ACK flag set to 1
 *      but the ACK number is the sequence number of the SYN packet + 1.
 *    This is because the client has received the SYN packet and is
 *    acknowledging it.
 *  A ACK packet has the ACK flag set to 1 and the ACK number is the sequence
 *    number of the SYN ACK packet + 1, acknowledging the SYN ACK packet.
 */

/* Top level TCP usage
 *  1. Create TCP (address, port, handler) -> tcp_server (server_socket,
 * server_address, handler)
 *
 *  2. Listen to TCP server socket -> listen_tcp_server_socket(server_socket)
 */

// Low level UDP functions
int create_server_socket(int port);
int bind_server_socket(int server_socket, int port);
int listen_server_socket(int server_socket);
int accept_connection(int server_socket);
int handle_connection(int connection_socket);
int close_connection(int connection_socket);
int close_server_socket(int server_socket);

// TCP
typedef struct {
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t payload_len;
    uint16_t flags;  // Flags for SYN, ACK, FIN etc.
    char payload[MAX_PAYLOAD_SIZE];
} tcp_packet;

typedef int (*tcp_connection_handler)(int connection_socket);

typedef struct {
    int server_socket_fd;
    struct sockaddr_in server_address;
    tcp_connection_handler handler;
} tcp_server;

typedef struct {
    int server_socket;
    int connection_socket;
} tcp_connection;

typedef enum {
    TCP_FLAG_SYN = 0x02,
    TCP_FLAG_ACK = 0x10,
    TCP_FLAG_DATA = 0x00,
    TCP_FLAG_FIN = 0x01,
    TCP_FLAG_FIN_ACK = 0x11,
} tcp_flag;

// TCP high level functions
tcp_server *create_tcp_server(char *address, int port,
                              tcp_connection_handler handler);
void listen_tcp_server(tcp_server *server);

// TCP low level functions

// create_tcp_server_socket creates a TCP server socket and returns the socket
int create_tcp_server_socket(char *address, int port,
                             tcp_connection_handler handler);
// bind_tcp_server_socket binds the TCP server socket to the address and port
int bind_tcp_server_socket(int server_socket, int port);
// listen_tcp_server_socket listens to the TCP server socket
int listen_tcp_server_socket(int server_socket);
// accept_tcp_connection accepts a TCP connection and returns the connection
int accept_tcp_connection(int server_socket);
// handle_tcp_connection handles a TCP connection_socket
int handle_tcp_connection(int connection_socket);
// close_tcp_connection closes a TCP connection
int close_tcp_connection(int connection_socket);
// close_tcp_server_socket closes a TCP server socket
int close_tcp_server_socket(int server_socket);

// TCP helper functions
int send_tcp_packet(int connection_socket, tcp_packet packet);
int receive_tcp_packet(int connection_socket, tcp_packet *packet);

// TCP packet functions
tcp_packet create_tcp_packet(uint32_t seq_no, uint32_t ack_no,
                             uint16_t payload_len, uint16_t flags,
                             char *payload);
tcp_packet create_tcp_syn_packet(uint32_t seq_no, uint32_t ack_no);
tcp_packet create_tcp_ack_packet(uint32_t seq_no, uint32_t ack_no);
tcp_packet create_tcp_fin_packet(uint32_t seq_no, uint32_t ack_no);
tcp_packet create_tcp_data_packet(uint32_t seq_no, uint32_t ack_no,
                                  char *payload);
tcp_packet create_tcp_fin_ack_packet(uint32_t seq_no, uint32_t ack_no);

// TCP packet helper functions
int is_tcp_syn_packet(tcp_packet packet);
int is_tcp_ack_packet(tcp_packet packet);
int is_tcp_fin_packet(tcp_packet packet);
int is_tcp_data_packet(tcp_packet packet);
int is_tcp_fin_ack_packet(tcp_packet packet);

// TCP packet printing functions
void print_tcp_packet(tcp_packet packet);
void print_tcp_syn_packet(tcp_packet packet);
void print_tcp_ack_packet(tcp_packet packet);
void print_tcp_fin_packet(tcp_packet packet);
void print_tcp_data_packet(tcp_packet packet);
void print_tcp_fin_ack_packet(tcp_packet packet);

// TCP packet functions
tcp_packet create_tcp_packet(uint32_t seq_no, uint32_t ack_no,
                             uint16_t payload_len, uint16_t flags,
                             char *payload) {
    tcp_packet packet;
    packet.seq_no = seq_no;
    packet.ack_no = ack_no;
    packet.payload_len = payload_len;
    packet.flags = flags;
    memcpy(packet.payload, payload, payload_len);
    return packet;
}

tcp_packet create_tcp_syn_packet(uint32_t seq_no, uint32_t ack_no) {
    return create_tcp_packet(seq_no, ack_no, 0, 0x02, NULL);
}

tcp_packet create_tcp_ack_packet(uint32_t seq_no, uint32_t ack_no) {
    return create_tcp_packet(seq_no, ack_no, 0, 0x10, NULL);
}

tcp_packet create_tcp_fin_packet(uint32_t seq_no, uint32_t ack_no) {
    return create_tcp_packet(seq_no, ack_no, 0, 0x01, NULL);
}

tcp_packet create_tcp_data_packet(uint32_t seq_no, uint32_t ack_no,
                                  char *payload) {
    return create_tcp_packet(seq_no, ack_no, strlen(payload), 0x00, payload);
}

tcp_packet create_tcp_fin_ack_packet(uint32_t seq_no, uint32_t ack_no) {
    return create_tcp_packet(seq_no, ack_no, 0, 0x11, NULL);
}

// TCP packet helper functions
int is_tcp_syn_packet(tcp_packet packet) {
    return packet.flags == TCP_FLAG_SYN;
}

int is_tcp_ack_packet(tcp_packet packet) {
    return packet.flags == TCP_FLAG_ACK;
}

int is_tcp_fin_packet(tcp_packet packet) {
    return packet.flags == TCP_FLAG_FIN;
}

int is_tcp_data_packet(tcp_packet packet) {
    return packet.flags == TCP_FLAG_DATA && packet.payload_len > 0;
}

int is_tcp_fin_ack_packet(tcp_packet packet) {
    return packet.flags == TCP_FLAG_FIN_ACK;
}

// TCP packet printing functions
void print_tcp_packet(tcp_packet packet) {
    printf("TCP Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
    printf("Payload Length: %d\n", packet.payload_len);
    printf("Flags: %d\n", packet.flags);
    printf("Payload: %s\n", packet.payload);
}

void print_tcp_syn_packet(tcp_packet packet) {
    printf("TCP SYN Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
}

void print_tcp_ack_packet(tcp_packet packet) {
    printf("TCP ACK Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
}

void print_tcp_fin_packet(tcp_packet packet) {
    printf("TCP FIN Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
}

void print_tcp_data_packet(tcp_packet packet) {
    printf("TCP Data Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
    printf("Payload Length: %d\n", packet.payload_len);
    printf("Payload: %s\n", packet.payload);
}

void print_tcp_fin_ack_packet(tcp_packet packet) {
    printf("TCP FIN ACK Packet:\n");
    printf("Seq No: %d\n", packet.seq_no);
    printf("Ack No: %d\n", packet.ack_no);
}

// TCP high level functions
tcp_server *create_tcp_server(char *address, int port,
                              tcp_connection_handler handler) {
    tcp_server *server = malloc(sizeof(tcp_server));
    server->server_socket_fd = create_tcp_server_socket(address, port, handler);
    server->handler = handler;
    return server;
}

void listen_tcp_server(tcp_server *server) {
    listen_tcp_server_socket(server->server_socket_fd);

    // end of TCP server
    free(server);  // free from heap, since it was malloc'd
}

// TCP low level functions
int create_tcp_server_socket(char *address, int port,
                             tcp_connection_handler handler) {
    int server_socket_fd = create_server_socket(port);
    bind_server_socket(server_socket_fd, port);
    listen_server_socket(server_socket_fd);
    return server_socket_fd;
}

int bind_tcp_server_socket(int server_socket, int port) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);  // Convert to network byte order
    server_address.sin_addr.s_addr = INADDR_ANY;
    memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

    int bind_status = bind(server_socket, (struct sockaddr *)&server_address,
                           sizeof(server_address));
    if (bind_status < 0) {
        perror("Error binding server socket");
        exit(1);
    }
    return bind_status;
}

int listen_tcp_server_socket(int server_socket) {
    int listen_status = listen(server_socket, MAX_CONNECTIONS);
    if (listen_status < 0) {
        perror("Error listening to server socket");
        exit(1);
    }
    return listen_status;
}

int accept_tcp_connection(int server_socket) {
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int connection_socket = accept(
        server_socket, (struct sockaddr *)&client_address, &client_address_len);
    if (connection_socket < 0) {
        perror("Error accepting connection");
        exit(1);
    }
    return connection_socket;
}

int handle_tcp_connection(int connection_socket) {
    tcp_packet syn_packet;
    receive_tcp_packet(connection_socket, &syn_packet);
    print_tcp_syn_packet(syn_packet);

    tcp_packet syn_ack_packet = create_tcp_syn_packet(0, 0);
    send_tcp_packet(connection_socket, syn_ack_packet);
    print_tcp_syn_packet(syn_ack_packet);

    tcp_packet ack_packet;
    receive_tcp_packet(connection_socket, &ack_packet);
    print_tcp_ack_packet(ack_packet);

    tcp_packet data_packet;
    receive_tcp_packet(connection_socket, &data_packet);
    print_tcp_data_packet(data_packet);

    tcp_packet fin_packet = create_tcp_fin_packet(0, 0);
    send_tcp_packet(connection_socket, fin_packet);
    print_tcp_fin_packet(fin_packet);

    tcp_packet fin_ack_packet;
    receive_tcp_packet(connection_socket, &fin_ack_packet);
    print_tcp_fin_ack_packet(fin_ack_packet);

    return 0;
}

int close_tcp_connection(int connection_socket) {
    int close_status = close(connection_socket);
    if (close_status < 0) {
        perror("Error closing connection");
        exit(1);
    }
    return close_status;
}

int close_tcp_server_socket(int server_socket) {
    int close_status = close(server_socket);
    if (close_status < 0) {
        perror("Error closing server socket");
        exit(1);
    }
    return close_status;
}

// Command line parsing
typedef enum {
    CMD_FLAG_TYPE_STRING,
    CMD_FLAG_TYPE_INT,
    CMD_FLAG_TYPE_BOOL,
} cmd_flag_type;

typedef int (*cmd_flag_validator)(char *flag_value);

typedef struct {
    char flag_name[1024];
    char flag_value[1024];
    cmd_flag_type flag_type;
    cmd_flag_validator flag_validator;
} cmd_flag;

typedef struct {
    cmd_flag cmd_flags[1024];
    hashmap *cmd_flags_map;
    int cmd_flags_count;
} cmd_flags;

typedef struct {
    char helper[1024];
    cmd_flags *flags;
} cmd;

cmd *create_cmd() {
    cmd *command = malloc(sizeof(cmd));
    command->flags = malloc(sizeof(cmd_flags));
    command->flags->cmd_flags_count = 0;
    command->flags->cmd_flags_map = init_hashmap();
    return command;
}

void print_cmd_help(cmd command) {
    printf("Usage: %s\n", command.helper);
    printf("Flags:\n");
    for (int i = 0; i < command.flags->cmd_flags_count; i++) {
        printf("%s %s\n", command.flags->cmd_flags[i].flag_name,
               command.flags->cmd_flags[i].flag_value);
    }
}

int cmd_flag_exists(cmd command, char *flag_name) {
    return hashmap_get(&command.flags->cmd_flags_map, flag_name) != NULL;
}

int validate_cmd_flag_type(cmd_flag flag, char *flag_value) {
    if (flag_value == NULL) {
        return 0;
    }

    if (strlen(flag_value) == 0) {
        return 0;
    }

    if (flag.flag_validator != NULL) {
        return flag.flag_validator(flag_value);
    }

    switch (flag.flag_type) {
        case CMD_FLAG_TYPE_INT:
            for (int i = 0; i < strlen(flag_value); i++) {
                if (flag_value[i] < '0' || flag_value[i] > '9') {
                    return 0;
                }
            }
            break;
        case CMD_FLAG_TYPE_BOOL:
            if (strcmp(flag_value, "true") != 0 &&
                strcmp(flag_value, "false") != 0) {
                return 0;
            }
            break;
        case CMD_FLAG_TYPE_STRING:
            break;
        default:
            break;
    }

    return 1;
}

// add_cmd_flag adds a flag to the cmd struct and to the hashmap for later use
// if any error then it will exit the program
void add_cmd_flag(cmd *command, char *flag_name, char *flag_value) {
    if (command->flags->cmd_flags_count >= 1024) {
        perror("Too many flags");
        exit(1);
    }

    if (cmd_flag_exists(*command, flag_name)) {
        perror("Flag already exists");
        exit(1);
    }

    strcpy(command->flags->cmd_flags[command->flags->cmd_flags_count].flag_name,
           flag_name);
    strcpy(
        command->flags->cmd_flags[command->flags->cmd_flags_count].flag_value,
        flag_value);
    hashmap_put(command->flags->cmd_flags_map, flag_name, flag_value);
    command->flags->cmd_flags_count++;
}

// parse_cmd parses the command line arguments and returns a cmd struct
// if any error then it will exit the program
//
// parse_cmd will also free the cmd struct as it should no longer be used.
int *parse_cmd(cmd *command, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (i + 1 >= argc) {
                perror("Flag value not provided");
                exit(1);
            }
            add_cmd_flag(command, argv[i], argv[i + 1]);
            i++;
        } else {
            strcpy(command->helper, argv[i]);
        }
    }

    free(command);

    return 0;
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    cmd *command = create_cmd();
    add_cmd_flag(command, "-p", "Port number");
    parse_cmd(command, argc, argv);

    // Get port number
    int port = atoi(hashmap_get(command->flags->cmd_flags_map, "-p"));

    // Create TCP server
    tcp_server *server = create_tcp_server(port);

    // Bind TCP server socket
    bind_tcp_server_socket(server->server_socket, port);

    // Listen to TCP server socket
    listen_tcp_server_socket(server->server_socket);

    // Accept TCP connection
    server->connection_socket = accept_tcp_connection(server->server_socket);

    // Handle TCP connection
    handle_tcp_connection(server->connection_socket);

    // Close TCP connection
    close_tcp_connection(server->connection_socket);
}

