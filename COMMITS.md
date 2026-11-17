# 📝 Padrão de commits

Este projeto usa commits com **gitmoji**: um emoji no início da mensagem indica o tipo da mudança, seguido de uma frase curta no imperativo.

```
<emoji> <resumo curto da mudança>
```

## Emojis usados

| Emoji | Quando usar | Exemplo |
| --- | --- | --- |
| ✨ | Nova funcionalidade | `✨ Adiciona tela de cotação` |
| 🐛 | Correção de bug | `🐛 Corrige fuso horário do relógio` |
| ⚡ | Melhoria de desempenho | `⚡ Reduz o tempo de atualização da tela` |
| 🔒 | Segurança e privacidade | `🔒 Move Wi-Fi e coordenadas para o secrets.h` |
| 📝 | Documentação | `📝 Atualiza o README com a ligação do OLED` |
| 🎉 | Commit inicial | `🎉 Primeiro commit` |
| ♻️ | Refatoração sem mudar comportamento | `♻️ Separa o desenho das telas em funções` |
| 🔧 | Configuração | `🔧 Ajusta .gitignore` |
| 🔥 | Remoção de código ou arquivos | `🔥 Remove tela não utilizada` |

## Regras

- Uma mudança lógica por commit.
- Resumo com até ~70 caracteres, sem ponto final.
- Nunca commitar o `secrets.h` (senha do Wi-Fi e localização). Ele já está no `.gitignore`.
