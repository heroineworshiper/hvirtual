#include "firehose.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>

union semun {
   int val; /* used for SETVAL only */
   struct semid_ds *buf; /* for IPC_STAT and IPC_SET */
   ushort *array;  /* used for GETALL and SETALL */
};

static void lock_sema(int semaphore)
{
	struct sembuf sop;
	
// decrease the semaphore
	sop.sem_num = 0;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	if(semop(semaphore, &sop, 1) < 0) perror("lock_sema");
}

static void unlock_sema(int semaphore)
{
	struct sembuf sop;
	
// decrease the semaphore
	sop.sem_num = 0;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	if(semop(semaphore, &sop, 1) < 0) perror("unlock_sema");
}

static int byteorder()
{
	int32_t byteordertest;
	int byteorder;

	byteordertest = 0x00000001;
	byteorder = *((unsigned char *)&byteordertest);
	return byteorder;
}


static void init_sockaddr(struct sockaddr_in *name, char *address)
{
	struct hostent *hostinfo;
	char hostname[1024];
	char *ptr;
	int port;

	ptr = strchr(address, ':');
	if(ptr)
	{
		ptr++;
		port = atol(ptr);
	}
	strcpy(hostname, address);
	ptr = strchr(hostname, ':');
	if(ptr)
	{
		*ptr = 0;
	}

	name->sin_family = AF_INET;
	name->sin_port = htons(port);
	hostinfo = gethostbyname(hostname);
	if(hostinfo == NULL) 
    {
    	fprintf (stderr, "init_sockaddr: unknown host %s.\n", hostname);
    	exit(1);
    }
	name->sin_addr = *(struct in_addr *)hostinfo->h_addr;
}

static void encode_packet(firehose_transmitter_t *transmitter)
{
	firehose_t *hose = (firehose_t*)transmitter->hose;
	firehose_header_t *header = (firehose_header_t*)transmitter->packet;

	header->end_of_file = hose->end_of_file;
	header->packet_id = hose->packet_id;
	header->payload_size = hose->payload_size;
	transmitter->packet_size = hose->payload_size + sizeof(firehose_header_t);

	if(!hose->end_of_file) memcpy(transmitter->packet + sizeof(firehose_header_t), 
		hose->payload, 
		hose->payload_size);

	if(byteorder()) SWAP_HEADER(header);
}

void transmitter_loop(void *ptr)
{
	firehose_transmitter_t *transmitter = (firehose_transmitter_t*)ptr;
	firehose_t *hose = (firehose_t*)transmitter->hose;
	
	while(!transmitter->done)
	{
		int bytes_written;

// Encode a packet
		pthread_mutex_lock(&hose->output_lock);

		encode_packet(transmitter);

		if(hose->end_of_file) transmitter->done = 1;
		pthread_mutex_unlock(&hose->input_lock);


// Write the packet
		bytes_written = write(transmitter->socket_fd,
			transmitter->packet,
			transmitter->packet_size);

		if(bytes_written != transmitter->packet_size)
		{
			hose->error = 1;
			perror("transmitter_loop: write");
		}
	}
}

static int new_transmitter(firehose_t *hose)
{
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;
	firehose_transmitter_t *transmitter = calloc(1, sizeof(firehose_transmitter_t));
	int result = 0;

	transmitter->hose = hose;
	pthread_attr_init(&attr);
	if((transmitter->socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
//	if((transmitter->socket_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("new_transmitter: socket");
		result = 1;
	}

	if(!result)
	{
		init_sockaddr(&transmitter->addr, 
			hose->destinations[hose->total_transmitters]);
		if(connect(transmitter->socket_fd, 
			(struct sockaddr*)&transmitter->addr, 
			sizeof(transmitter->addr)) < 0)
		{
			perror("new_transmitter: connect");
			result = 1;
		}
	}

	if(!result)
	{
		pthread_create(&transmitter->tid, 
			&attr, 
			(void*)transmitter_loop, 
			transmitter);
	}

	hose->transmitters[hose->total_transmitters++] = transmitter;	
	return result;
}

static void delete_transmitter(firehose_transmitter_t *transmitter)
{
	pthread_join(transmitter->tid, 0);
	close(transmitter->socket_fd);
	free(transmitter);
}

static int read_socket(int socket_fd, char *data, int len)
{
	int bytes_read = 0;
	int offset = 0;

	while(len > 0 && bytes_read >= 0)
	{
		bytes_read = read(socket_fd, data + offset, len);
		if(bytes_read > 0)
		{
			len -= bytes_read;
			offset += bytes_read;
		}
	}
	return offset;
}


static void port_loop(void *ptr)
{
	firehose_port_receiver_t *port = (firehose_port_receiver_t*)ptr;
	firehose_receiver_t *receiver = (firehose_receiver_t*)port->receiver;
	firehose_t *hose = (firehose_t*)port->hose;
	struct sockaddr_in clientname;
	socklen_t size = sizeof(clientname);
	int result = 0;
	char *packet = malloc(PAYLOAD_SIZE + sizeof(firehose_header_t));
	long packet_size;
	int i;

// Open the port and wait on it if not already done
	if(port->number > 0)
	{
		if(listen(hose->socket_fd[port->number], 1) < 0)
    	{
    		perror("port_loop: listen");
    		result = 1;
    	}

		if(!result)
		{
    		if((port->socket_fd = accept(hose->socket_fd[port->number],
                		(struct sockaddr*)&clientname, 
						&size)) < 0)
    		{
        		perror("port_loop: accept");
        		result = 1;
    		}
		}
	}

// May now resume listening on master socket
	pthread_mutex_unlock(&port->startup_lock);

// Read packets
	while(port->state != PORT_DONE)
	{
		firehose_header_t *header = (firehose_header_t*)packet;
		int next_packet;


		if(port->state == PORT_RECEIVING)
		{
// Read packet header from network
			read_socket(port->socket_fd, packet, sizeof(firehose_header_t));
			if(byteorder()) SWAP_HEADER(packet);

//printf("port_loop 1 %d\n", header->end_of_file);
// Read payload from network
			if(port->state == PORT_RECEIVING &&
				!header->end_of_file)
			{
				result = !read_socket(port->socket_fd, 
					packet + sizeof(firehose_header_t), 
					header->payload_size);

//printf("port_loop 1 %02x\n", *(packet + sizeof(firehose_header_t)));
				if(result) receiver->error = result;

// Wait for packet to become available for writing new one
				if(!result) pthread_mutex_lock(&receiver->input_lock);
			}
		}




		pthread_mutex_lock(&receiver->packet_lock);




// Update state
		if(header->end_of_file)
		{
			int total;

			port->state = PORT_DONE;
			total = 0;
			for(i = 0; i < hose->total_receivers; i++)
			{
				if(receiver->ports[i]->state == PORT_DONE)
					total++;
			}

// Promote receiver state to current
			if(total == hose->total_receivers) receiver->loop_state = PORT_DONE;

			close(port->socket_fd);
		}



// Store packet
		if(port->state == PORT_RECEIVING)
		{
// Get next available packet
			for(i = 0; i < receiver->total_allocated; i++)
			{
				if(!receiver->packet_filled[i]) 
				{
					next_packet = i;
					break;
				}
			}

			receiver->packet_buffer[next_packet] = 
				realloc(receiver->packet_buffer[next_packet],
					sizeof(firehose_header_t) + PAYLOAD_SIZE);

			memcpy(receiver->packet_buffer[next_packet], 
				packet,
				sizeof(firehose_header_t) + header->payload_size);

// Increase counters
			receiver->packet_filled[next_packet] = 1;
			receiver->total_bytes_received += header->payload_size;
			receiver->total_filled++;

// More room still available
			if(receiver->total_filled < receiver->total_allocated)
				pthread_mutex_unlock(&receiver->input_lock);
		}

// Release buffer reader for reading.
		pthread_mutex_unlock(&receiver->output_lock);
		pthread_mutex_unlock(&receiver->packet_lock);
	}
	free(packet);
}

static int new_receiver_port(firehose_t *hose, 
	firehose_receiver_t *receiver,
	firehose_port_receiver_t **port,
	int number,
	int port0_socket_fd)
{
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_attr_init(&attr);

	(*port) = calloc(1, sizeof(firehose_port_receiver_t));
	(*port)->hose = hose;
	(*port)->receiver = receiver;
	(*port)->number = number;
	if(number == 0) (*port)->socket_fd = port0_socket_fd;

	pthread_mutex_init(&(*port)->startup_lock, &mutex_attr);
	pthread_mutex_lock(&(*port)->startup_lock);

	pthread_create(&(*port)->tid, 
		&attr, 
		(void*)port_loop, 
		(*port));
	return 0;
}

void delete_receiver_port(firehose_port_receiver_t *port)
{
	pthread_join(port->tid, NULL);
	free(port);
}


static int new_receiver_engine(firehose_t *hose, 
	firehose_receiver_t **receiver,
	int port0_socket_fd)
{
	int i, result = 0;
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);

	(*receiver) = calloc(1, sizeof(firehose_receiver_t));
	(*receiver)->hose = hose;

	pthread_mutex_init(&(*receiver)->output_lock, &mutex_attr);
	pthread_mutex_init(&(*receiver)->input_lock, &mutex_attr);
	pthread_mutex_init(&(*receiver)->packet_lock, &mutex_attr);
	pthread_mutex_lock(&(*receiver)->output_lock);

	(*receiver)->packet_buffer = calloc(1, sizeof(char*) * DEFAULT_PACKETS);
	(*receiver)->packet_filled = calloc(1, sizeof(int) * DEFAULT_PACKETS);
	(*receiver)->total_allocated = DEFAULT_PACKETS;
	(*receiver)->output_state = PORT_RECEIVING;
	(*receiver)->loop_state = PORT_RECEIVING;

	for(i = 0; i < hose->total_receivers; i++)
	{
		result |= new_receiver_port(hose,
			(*receiver), 
			&(*receiver)->ports[i], 
			i,
			port0_socket_fd);
	}

// Send start code on first port which causes the client
// to connect to the remaining ports and start writing now.
	if(!result)
	{
		char start_code = START_CODE;
		result = !write((*receiver)->ports[0]->socket_fd, &start_code, 1);
	}

// Wait for port receivers to start before resuming listening on sockets
	for(i = 0; i < hose->total_receivers; i++)
	{
		pthread_mutex_lock(&(*receiver)->ports[i]->startup_lock);
		pthread_mutex_unlock(&(*receiver)->ports[i]->startup_lock);
	}


	return result;
}




















firehose_t* firehose_new()
{
	return calloc(1, sizeof(firehose_t));
}

void firehose_delete(firehose_t *hose)
{
	int i;

	if(hose->state == STATE_TRANSMITTING)
	{
		for(i = 0; i < hose->total_transmitters; i++)
		{
			pthread_mutex_lock(&hose->input_lock);
			hose->packet_id++;
			hose->end_of_file = 1;
			hose->payload_size = 0;
			pthread_mutex_unlock(&hose->output_lock);
		}

		for(i = 0; i < hose->total_transmitters; i++)
		{
			delete_transmitter(hose->transmitters[i]);
		}
		
		pthread_mutex_destroy(&hose->input_lock);
		pthread_mutex_destroy(&hose->output_lock);
	}
	
	if(hose->state == STATE_RECEIVING)
	{
		for(i = 0; i < hose->total_receivers; i++)
		{
			close(hose->socket_fd[i]);
		}
	}

	for(i = 0; i < hose->total_destinations; i++)
	{
		free(hose->destinations[i]);
	}

	free(hose);
}

void firehose_set_udp(firehose_t *hose, int value)
{
	hose->use_udp = value;
}

void firehose_delete_receiver(firehose_receiver_t *receiver)
{
	int i;
	union semun arg;
	firehose_t *hose = (firehose_t*)receiver->hose;

	for(i = 0; i < hose->total_receivers; i++)
	{
		delete_receiver_port(receiver->ports[i]);
	}

	pthread_mutex_destroy(&receiver->packet_lock);
	pthread_mutex_destroy(&receiver->input_lock);
	pthread_mutex_destroy(&receiver->output_lock);

	for(i = 0; i < receiver->total_allocated; i++)
	{
		if(receiver->packet_buffer[i]) 
			free(receiver->packet_buffer[i]);
	}
	
	free(receiver->packet_buffer);
	free(receiver->packet_filled);
	free(receiver);
}


void firehose_add_destination(firehose_t *hose, char *hostname)
{
	if(hose->total_destinations >= MAX_PORTS)
	{
		fprintf(stderr, "firehose_add_destination: MAX_PORTS exceeded.\n");
		return;
	}

	hose->destinations[hose->total_destinations] = calloc(1, strlen(hostname) + 1);
	strcpy(hose->destinations[hose->total_destinations], hostname);
	hose->total_destinations++;
}

void firehose_add_receiver(firehose_t *hose, int port)
{
	if(hose->total_receivers > MAX_PORTS)
	{
		fprintf(stderr, "firehose_add_receiver: MAX_PORTS exceeded.\n");
		return;
	}
	
	hose->receivers[hose->total_receivers++] = port;
}

int firehose_open_write(firehose_t *hose)
{
	int i, result = 0;
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutex_init(&hose->input_lock, &mutex_attr);
	pthread_mutex_init(&hose->output_lock, &mutex_attr);
	pthread_mutex_lock(&hose->output_lock);
	
	hose->packet_id = -1;

// Create first transmitter
	result = new_transmitter(hose);

// Wait for start code
	if(!result)
	{
		char start_code;
		result = !read(hose->transmitters[0]->socket_fd, &start_code, 1);
		if(result)
		{
			fprintf(stderr, "firehose_open_write read: %s\n", strerror(errno));
		}
		if(start_code != START_CODE)
		{
			fprintf(stderr, "firehose_open_write: invalid start code %02x\n", start_code);
			result = 1;
		}
	}

// Connect remaining transmitters
	if(!result)
	{
		for(i = 1; i < hose->total_destinations; i++)
		{
			result |= new_transmitter(hose);
		}
	}

	if(!result)
	{
		hose->state = STATE_TRANSMITTING;
	}


	return result;
}

long firehose_write_data(firehose_t *hose, char *data, long data_size)
{
	int i;

	for(i = 0; i < data_size; i += PAYLOAD_SIZE)
	{
		int fragment_size = PAYLOAD_SIZE;
		if(fragment_size + i > data_size)
		{
			fragment_size = data_size - i;
		}

		pthread_mutex_lock(&hose->input_lock);
		hose->packet_id++;
		hose->payload_size = fragment_size;
		memcpy(hose->payload, data + i, fragment_size);
		pthread_mutex_unlock(&hose->output_lock);
	}

	return hose->error ? 0 : data_size;
}

int firehose_open_read(firehose_t *hose, firehose_receiver_t **receiver)
{
	int result = 0;
	int port0_socket_fd;
	int i;

// Open first port
	if(hose->state == STATE_CLOSED)
	{
		for(i = 0; i < hose->total_receivers; i++)
		{
			if(hose->use_udp)
				hose->socket_fd[i] = socket(PF_INET, SOCK_DGRAM, 0);
			else
				hose->socket_fd[i] = socket(PF_INET, SOCK_STREAM, 0);

			if(hose->socket_fd[i] < 0)
			{
				perror("firehose_open_read: socket");
				result = 1;
			};

			if(!result)
			{
				hose->addr.sin_family = AF_INET;
				hose->addr.sin_port = htons((unsigned short)hose->receivers[i]);
				hose->addr.sin_addr.s_addr = htonl(INADDR_ANY);

				if(bind(hose->socket_fd[i], 
					(struct sockaddr*)&hose->addr, 
					sizeof(hose->addr)) < 0)
				{
					perror("firehose_open_read: bind");
					result = 1;
				}
			}
		}
	}
	else
	{
		if(hose->use_udp) sleep(1000000);
	}

	if(!result)
	{
		hose->state = STATE_RECEIVING;
	}

// Listen and accept on first port, then handshake the remaining ports
	if(!result && !hose->use_udp)
	{
		if(listen(hose->socket_fd[0], 1) < 0)
    	{
    		perror("firehose_open_read: listen");
    		result = 1;
    	}
	}

	if(!result && !hose->use_udp)
	{
		struct sockaddr_in clientname;
		socklen_t size = sizeof(clientname);

    	if((port0_socket_fd = accept(hose->socket_fd[0],
                	(struct sockaddr*)&clientname, 
					&size)) < 0)
    	{
        	perror("port_loop: accept");
        	result = 1;
    	}
		else
		{
/*
 * 			fprintf(stderr, "Connection from host %s port %d.\n", 
 * 				inet_ntoa(clientname.sin_addr),
 * 				ntohs(clientname.sin_port));
 */
		}
	}


	if(!result)
	{
		result = new_receiver_engine(hose, receiver, port0_socket_fd);
	}

	return result;
}

long firehose_read_data(firehose_receiver_t *receiver, 
	char *data, 
	long data_size)
{
	long result = 0;
	int i = 0;

	if(receiver->error) return result;

	while(i < data_size && 
		!receiver->error &&
		!firehose_eof(receiver))
	{
// Try spill first
		if(receiver->spill_size)
		{
			long fragment_size = receiver->spill_size;
			int j;

			if(fragment_size > data_size)
				fragment_size = data_size;

			memcpy(data + i, receiver->spill, fragment_size);

			for(j = fragment_size; j < receiver->spill_size; j++)
			{
				receiver->spill[j - fragment_size] = receiver->spill[j];
			}
			i += fragment_size;
			receiver->spill_size -= fragment_size;
			result += fragment_size;
			receiver->total_bytes_read += fragment_size;
		}
		else
// Read packets
		{
			firehose_header_t *header;
			char *packet_buffer = 0;
			int j, next_packet;

// Wait for new packet to become available
			if(receiver->output_state == PORT_RECEIVING)
				pthread_mutex_lock(&receiver->output_lock);

			pthread_mutex_lock(&receiver->packet_lock);

			receiver->output_state = receiver->loop_state;

			for(j = 0; j < receiver->total_allocated; j++)
			{
				if(receiver->packet_filled[j])
				{
					header = (firehose_header_t*)receiver->packet_buffer[j];
//printf("%d ", header->packet_id);
					if(header->packet_id == receiver->current_id)
					{
						packet_buffer = receiver->packet_buffer[j];
						next_packet = j;
						break;
					}
				}
			}
//printf("\n");

// Allocate more packet buffers
			if(!packet_buffer && receiver->total_filled >= receiver->total_allocated)
			{
				receiver->packet_buffer = realloc(receiver->packet_buffer,
					sizeof(char*) * (receiver->total_allocated + 1));
				receiver->packet_filled = realloc(receiver->packet_filled,
					sizeof(int) * (receiver->total_allocated + 1));
				receiver->packet_filled[receiver->total_allocated] = 0;
				receiver->packet_buffer[receiver->total_allocated] = 0;
				receiver->total_allocated++;
				pthread_mutex_unlock(&receiver->input_lock);
			}

// Load spill buffer with new packet
			if(packet_buffer)
			{
				memcpy(receiver->spill, 
					packet_buffer + sizeof(firehose_header_t), 
					header->payload_size);
				receiver->spill_size = header->payload_size;

				receiver->packet_filled[next_packet] = 0;
				receiver->current_id++;
				receiver->total_filled--;
				pthread_mutex_unlock(&receiver->input_lock);
			}

			pthread_mutex_unlock(&receiver->packet_lock);
		}
	}

//printf("read_data 2\n");


	return result;
}

int firehose_eof(firehose_receiver_t *receiver)
{
	int result;
	result = (receiver->output_state == PORT_DONE) && 
		(receiver->total_bytes_read >= receiver->total_bytes_received);
	return result;
}


