# Robô Seguidor de Linha por Cor

Projeto de robô móvel seguidor de linha que identifica e segue linhas coloridas com sistema de prioridade, desenvolvido para a **BitDogLab**.

## Descrição

Este projeto implementa um robô autônomo que:
- **Segue linhas coloridas** com prioridade: 🟡 Amarelo > 🔴 Vermelho > 🔵 Azul
- **Para quando detecta preto** (linha de parada)
- **Evita obstáculos** usando sensor ultrassônico
- **Indica status** através de LED RGB

## Hardware Necessário

| Componente | Quantidade | Descrição |
|------------|:----------:|-----------|
| BitDogLab | 1 | Microcontrolador principal |
| TCS34725 | 2 | Sensor de cor I2C (esquerdo/direito) |
| SR04 (módulo I2C) | 1 | Sensor ultrassônico para obstáculos |
| BitMovel | 1 | Driver de motor |
| Motor DC | 2 | Motores para as rodas |
| LED RGB | 1 | Indicador de status |
| Botão | 1 | Start/Stop |

## Pinagem

### I2C
| Bus | Função | SDA | SCL |
|-----|--------|:---:|:---:|
| I2C0 | Sensor de cor esquerdo | GP0 | GP1 |
| I2C1 | Sensor de cor direito + Ultrassônico | GP2 | GP3 |

### Motores
| Motor | Forward | Backward | PWM |
|-------|:-------:|:--------:|:---:|
| Esquerdo | GP9 | GP4 | GP8 |
| Direito | GP18 | GP19 | GP16 |
| Standby | GP20 | - | - |

### LED RGB
| Cor | Pino |
|-----|:----:|
| Verde | GP11 |
| Azul | GP12 |
| Vermelho | GP13 |

### Botão
- **Start/Stop**: GP5

## Como Usar

1. **Ligue o robô** - LEDs acendem indicando modo de espera
2. **Pressione o botão** (GP5) para iniciar
3. O robô começará a seguir as linhas coloridas
4. **Para parar**: coloque o robô sobre uma superfície preta

### Indicadores LED

| Cor LED | Significado |
|---------|-------------|
| 🔴🟢🔵 (Branco) | Modo de espera |
| 🟢 Verde | Seguindo linha |
| 🔵 Azul | Evitando obstáculo |
| 🔴🔵 Magenta | Parada (preto detectado) |

## Estrutura do Projeto

```
robo_movel/
├── main.c          # Aplicação principal e lógica de controle
├── config.h        # Configurações de hardware e constantes
├── led.c/h         # Controle do LED RGB
├── motor.c/h       # Controle dos motores DC
├── sensor.c/h      # Leitura dos sensores (cor e ultrassônico)
├── CMakeLists.txt  # Configuração de build
└── README.md       # Este arquivo
```

## Configurações Ajustáveis

No arquivo `config.h`, você pode ajustar:

```c
#define MOTOR_BASE_SPEED      17000   // Velocidade base (PWM)
#define MOTOR_SPIN_SPEED      15000   // Velocidade de curva
#define MOTOR_TURN_TIME_MS    650     // Tempo de curva 90°
#define COLOR_BLACK_THRESHOLD 40      // Sensibilidade para preto
#define OBSTACLE_DISTANCE_MM  200     // Distância de obstáculo (mm)
#define COLOR_LOCK_TIME_MS    500     // Tempo de lock de prioridade
```

## Sistema de Prioridade de Cores

O robô usa um sistema inteligente para decidir qual linha seguir quando detecta múltiplas cores:

1. **Amarelo** (prioridade máxima) - Sempre seguido
2. **Vermelho** (prioridade média) - Seguido se não houver amarelo
3. **Azul** (prioridade baixa) - Seguido se não houver vermelho ou amarelo
4. **Preto** (especial) - Causa parada imediata

## Licença

Projeto desenvolvido para fins educacionais no programa **Embarcatech**.

---
