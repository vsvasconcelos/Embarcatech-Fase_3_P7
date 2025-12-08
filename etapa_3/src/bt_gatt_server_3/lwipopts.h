#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Este arquivo contém as opções de configuração para a pilha de rede Lightweight IP (LwIP).
// O LwIP é uma pilha TCP/IP de código aberto projetada para sistemas embarcados com recursos limitados.
// Embora nosso projeto principal seja BLE, o Raspberry Pi Pico W possui Wi-Fi, e o LwIP é a pilha
// de rede subjacente frequentemente usada para a conectividade IP.
// Mesmo que `server.c` não use diretamente a rede IP, a inicialização do `cyw43_arch` pode depender
// de certas partes do LwIP para o funcionamento do chip.
// Consulte https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html para mais detalhes.

// --- Configurações Gerais do Sistema Operacional (OS) e Memória ---
#define NO_SYS                      1   // Desabilita o suporte a um sistema operacional real-time (RTOS).
                                        // Comum em ambientes bare-metal como o Pico SDK, onde o loop principal da aplicação gerencia tudo.
#define LWIP_SOCKET                 0   // Desabilita as APIs de sockets (Berkeley-like sockets).
                                        // Isso indica que a aplicação não está usando funcionalidades de rede IP de alto nível diretamente.
#define MEM_LIBC_MALLOC             0   // Indica que o LwIP não usará malloc/free da libc.
                                        // Em vez disso, usará seus próprios mecanismos de gerenciamento de memória (mempools),
                                        // que são mais eficientes e previsíveis em sistemas embarcados.
#define MEM_ALIGNMENT               4   // Alinhamento da memória em bytes (normalmente 4 para 32-bit).
#define MEM_SIZE                    4000 // Tamanho total da heap de memória para o LwIP (em bytes).
                                        // Um valor relativamente pequeno, esperado já que as funcionalidades IP estão em grande parte desabilitadas.

// --- Configurações de Buffers e Filas ---
#define MEMP_NUM_TCP_SEG            32  // Número de segmentos TCP no pool de memória.
#define MEMP_NUM_ARP_QUEUE          10  // Número de entradas na fila ARP (Address Resolution Protocol).
#define PBUF_POOL_SIZE              24  // Número de pbufs (packet buffers) disponíveis no pool.
                                        // Pbufs são estruturas de dados para gerenciar pacotes de rede.

// --- Habilitação de Protocolos Básicos ---
#define LWIP_ARP                    1   // Habilita o Address Resolution Protocol (ARP). Essencial para mapeamento MAC-IP.
#define LWIP_ETHERNET               1   // Habilita o suporte à camada Ethernet.
#define LWIP_ICMP                   1   // Habilita o Internet Control Message Protocol (ICMP), usado para diagnósticos (e.g., ping).
#define LWIP_RAW                    1   // Habilita a API RAW do LwIP, para acesso direto a pacotes.

// --- Configurações TCP (Transmission Control Protocol) ---
#define TCP_WND                     (8 * TCP_MSS)       // Tamanho da janela de recepção TCP.
#define TCP_MSS                     1460                // Tamanho máximo do segmento TCP.
#define TCP_SND_BUF                 (8 * TCP_MSS)       // Tamanho do buffer de envio TCP.
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS)) // Tamanho da fila de envio TCP.
#define LWIP_TCP_KEEPALIVE          1                   // Habilita a funcionalidade TCP Keep-Alive.

// --- Configurações de Interface de Rede (NETIF) ---
#define LWIP_NETIF_STATUS_CALLBACK  1   // Habilita callbacks para mudanças de status da interface de rede (e.g., IP atribuído).
#define LWIP_NETIF_LINK_CALLBACK    1   // Habilita callbacks para mudanças de status do link da interface (e.g., conexão Wi-Fi).
#define LWIP_NETIF_HOSTNAME         1   // Habilita a definição de um hostname para a interface de rede.
#define LWIP_NETIF_TX_SINGLE_PBUF   1   // Otimização para envio de pbufs únicos.

// --- Outros Protocolos e Funcionalidades de Rede ---
#define LWIP_NETCONN                0   // Desabilita a API Netconn. Semelhante a LWIP_SOCKET, indica que não estamos usando essas abstrações.
#define LWIP_CHKSUM_ALGORITHM       3   // Algoritmo de checksum a ser usado (específico para a arquitetura do Pico W).
#define LWIP_DHCP                   1   // Habilita o Dynamic Host Configuration Protocol (DHCP) para obter IPs automaticamente.
#define LWIP_IPV4                   1   // Habilita o suporte a IPv4.
#define LWIP_TCP                    1   // Habilita o suporte a TCP.
#define LWIP_UDP                    1   // Habilita o suporte a UDP.
#define LWIP_DNS                    1   // Habilita o suporte a DNS (Domain Name System) para resolução de nomes.
#define DHCP_DOES_ARP_CHECK         0   // Desabilita a verificação ARP durante o DHCP (pode acelerar a aquisição de IP).
#define LWIP_DHCP_DOES_ACD_CHECK    0   // Desabilita a verificação de detecção de conflito de endereço (ACD) para DHCP.

// --- Estatísticas e Depuração (Debug) ---
// Estatísticas de LwIP: Geralmente desabilitadas em produção para economizar memória e CPU.
#define MEM_STATS                   0   // Desabilita estatísticas de alocação de memória.
#define SYS_STATS                   0   // Desabilita estatísticas do sistema.
#define MEMP_STATS                  0   // Desabilita estatísticas do pool de memória.
#define LINK_STATS                  0   // Desabilita estatísticas da camada de link.

#ifndef NDEBUG
// Bloco de depuração: Estas flags são ativadas SOMENTE se o projeto NÃO estiver sendo compilado em modo de release (NDEBUG não definido).
// Isso é vital para identificar problemas de rede durante o desenvolvimento.
#define LWIP_DEBUG                  1   // Habilita mensagens de debug gerais do LwIP.
#define LWIP_STATS                  1   // Habilita todas as estatísticas de LwIP.
#define LWIP_STATS_DISPLAY          1   // Habilita a exibição das estatísticas.
#endif

// Flags de depuração específicas de cada módulo do LwIP.
// Mesmo que LWIP_DEBUG esteja habilitado, estas flags permitem um controle granular sobre qual saída de depuração é gerada.
// Todas estão definidas como LWIP_DBG_OFF (desligadas) por padrão para evitar um volume excessivo de logs,
// mas podem ser ativadas individualmente para depurar módulos específicos.
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

#endif /* _LWIPOPTS_H */