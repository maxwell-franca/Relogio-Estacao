# 🕒 Relógio Estação ESP32

**Estação de mesa com display OLED: hora certa por NTP, clima da sua cidade e cotação de dólar, euro e bitcoin, em telas que se alternam sozinhas.**

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00878F?style=for-the-badge&logo=arduino&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Licença](https://img.shields.io/badge/Licen%C3%A7a-MIT-green?style=for-the-badge)

---

## 🧠 O que é

Um projeto embarcado que reúne três informações do dia a dia em um único display de 128x64 pixels:

- 🕒 **Relógio NTP:** hora, data e dia da semana sincronizados pela internet, no fuso de Brasília.
- 🌤️ **Clima:** temperatura, sensação térmica, umidade, vento, mínima, máxima e chance de chuva, via [Open-Meteo](https://open-meteo.com/) (gratuito, sem chave de API).
- 💱 **Cotação:** dólar, euro e bitcoin em reais, com seta de alta ou baixa, via [AwesomeAPI](https://docs.awesomeapi.com.br/api-de-moedas) (gratuito, sem chave de API).

---

## ✨ Funcionalidades

- Três telas que se alternam a cada 6 segundos
- Botão **BOOT** (GPIO 0) para pular para a próxima tela
- Rodapé do relógio com temperatura atual e cotação do dólar
- Clima atualizado a cada 10 minutos e cotação a cada 5 minutos
- Nova tentativa automática em 30 segundos quando uma consulta falha
- Reconexão automática do Wi-Fi e indicador `!` no display quando está offline
- Varredura de redes na inicialização, que ajuda a descobrir se o problema é o nome da rede, a senha ou o sinal
- Dados pessoais (Wi-Fi e localização) separados do código em `secrets.h`, fora do Git

---

## 🔧 Hardware

| Componente | Observação |
| --- | --- |
| ESP32 | Placa de desenvolvimento (testado com "ESP32 Dev Module") |
| Display OLED SSD1306 | 128x64, I2C, endereço `0x3C` (alguns módulos usam `0x3D`) |
| Cabo USB | Para gravar e alimentar |
| Rede Wi-Fi 2.4 GHz | O ESP32 não conecta em redes 5 GHz |

### Ligação do display

| OLED | ESP32 |
| --- | --- |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3V3 |
| GND | GND |

---

## 📁 Estrutura

```
relogio-estacao/
├── relogio_estacao/
│   ├── relogio_estacao.ino   # código principal
│   └── secrets.example.h     # modelo dos dados pessoais
├── COMMITS.md                # padrão de commits do projeto
├── LICENSE
└── README.md
```

A pasta do sketch precisa ter o mesmo nome do arquivo `.ino` para a Arduino IDE abri-lo.

---

## 🚀 Como usar

1. **Clone o repositório**
   ```
   git clone https://github.com/maxwell-franca/relogio-estacao.git
   ```
2. **Instale as bibliotecas** pelo Gerenciador de Bibliotecas da Arduino IDE:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - ArduinoJson (versão **7.x**)
3. **Crie o seu `secrets.h`** a partir do modelo, dentro da pasta `relogio_estacao`:
   ```
   copy relogio_estacao\secrets.example.h relogio_estacao\secrets.h
   ```
   Abra o `secrets.h` e preencha o Wi-Fi, a latitude, a longitude e o nome da cidade. Esse arquivo está no `.gitignore` e **não vai para o GitHub**.
4. **Use as coordenadas do centro da sua cidade**, não do seu endereço. Elas são enviadas ao serviço de clima (veja [Privacidade](#-privacidade-e-segurança)).
5. **Confira o fuso horário** em `TZ_INFO` no `.ino`. O padrão é Brasília (`<-03>3`, sem horário de verão).
6. **Abra o `relogio_estacao.ino`**, selecione a placa **ESP32 Dev Module** e grave.
7. **Abra o Monitor Serial** (115200 baud) para acompanhar a conexão, o IP e o resultado de cada consulta.

---

## 🔒 Privacidade e segurança

O projeto não usa chaves de API nem contas. O que sai do aparelho:

| Para onde | O que vai |
| --- | --- |
| Open-Meteo (`api.open-meteo.com`) | Latitude e longitude com 2 casas decimais (precisão de cerca de 1 km) e o seu endereço IP |
| AwesomeAPI (`economia.awesomeapi.com.br`) | Apenas o seu endereço IP |
| Servidores NTP (`pool.ntp.org`, `time.google.com`, `a.st1.ntp.br`) | Consulta de hora, com o seu endereço IP |

A senha do Wi-Fi fica só no ESP32 e no seu `secrets.h`.

Cuidados:

- **Nunca envie o `secrets.h` ao GitHub.** Antes do `git add`, rode `git status` e confirme que ele não aparece.
- **O Monitor Serial mostra o IP local e as redes Wi-Fi vizinhas** (na varredura inicial). Revise antes de colar um log em fórum ou issue.
- **As consultas HTTPS não validam o certificado** (`setInsecure`). Isso é aceitável porque nenhum dado sensível é enviado, mas alguém com controle da sua rede local poderia exibir clima ou cotação falsos.

---

## ⚠️ Limitações

- **Atualizações bloqueiam a tela:** enquanto uma consulta roda (até cerca de 10 s se a internet estiver lenta), o relógio e o botão ficam parados.
- **Fuso do clima fixo:** a consulta usa `America/Sao_Paulo` para os valores de mínima e máxima do dia. Fora desse fuso, altere também a URL em `atualizarClima()`.
- **Sem acentos:** a fonte padrão do display não exibe acentos, então use `CIDADE` sem eles.
- **Serviços gratuitos de terceiros:** Open-Meteo e AwesomeAPI podem limitar ou mudar o acesso.

---

## 📝 Padrão de commits

Este projeto usa commits com gitmoji. Os detalhes estão em [COMMITS.md](COMMITS.md).

---

## 📄 Licença

Distribuído sob a licença **MIT**. Veja o arquivo [LICENSE](LICENSE).

---

## 👤 Autor

**Maxwell França** · Contador & Administrador | IA, Automação e Sistemas Embarcados  
🇧🇷 São Luís – MA, Brasil

<a href="https://github.com/maxwell-franca">
  <img src="https://img.shields.io/badge/GitHub-maxwell--franca-181717?style=for-the-badge&logo=github" />
</a>

---

⭐ Se gostou do projeto, deixe uma estrela no repositório
