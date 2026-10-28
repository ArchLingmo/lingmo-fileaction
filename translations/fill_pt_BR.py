#!/usr/bin/env python3
"""Fills lingmo-fileaction_pt_BR.ts from the table below (run after lupdate):
    lupdate -no-obsolete -locations none src qml -ts translations/lingmo-fileaction_pt_BR.ts
    python3 translations/fill_pt_BR.py translations/lingmo-fileaction_pt_BR.ts
The "Engine" strings are the English names of lingmo-fileaction-engine's slices (FileAction's Catalog.cs)."""
import sys
import xml.etree.ElementTree as ET

PT = {
    "Open with": "Abrir com",
    "That app is no longer installed.": "Esse app não está mais instalado.",
    # wheels, slices, dialogs
    "File actions": "Ações de arquivo",
    "Convert": "Converter",
    "Tools": "Ferramentas",
    "Actions": "Ações",
    "Repeat": "Repetir",
    "Cancel": "Cancelar",
    "Cancelled": "Cancelado",
    "Run": "Executar",
    "Open": "Abrir",
    "Show in folder": "Mostrar na pasta",
    "Drop files here": "Solte arquivos aqui",
    "Nothing on this wheel for these files. Tab shows the others.":
        "Nada nesta roda para estes arquivos. Tab mostra as outras.",
    "Copy to…": "Copiar para…",
    "Move to…": "Mover para…",
    "Copy to": "Copiar para",
    "Move to": "Mover para",
    "Copy to %1": "Copiar para %1",
    "Move to %1": "Mover para %1",
    "Copied to %1": "Copiado para %1",
    "Moved to %1": "Movido para %1",
    "Copy path": "Copiar caminho",
    "Path copied": "Caminho copiado",
    "Paths copied": "Caminhos copiados",
    "Terminal here": "Terminal aqui",
    "Share": "Compartilhar",
    "No terminal app was found.": "Nenhum app de terminal foi encontrado.",
    "Copied, but %1 could not be removed.": "Copiado, mas não foi possível remover %1.",
    "Could not create %1": "Não foi possível criar %1",
    "Clipboard": "Área de transferência",
    "Converting to %1": "Convertendo para %1",
    "Convert to %1": "Converter para %1",
    "Ready: %1": "Pronto: %1",
    "%n files": ["%n arquivo", "%n arquivos"],
    "%n files ready": ["%n arquivo pronto", "%n arquivos prontos"],
    "Could not finish: %1": "Não deu para concluir: %1",
    "Nothing to do": "Nada a fazer",
    "needs %1": "precisa de %1",
    "%1 is not installed. Install it in the Store or with pacman.":
        "%1 não está instalado. Instale pela Loja ou com o pacman.",
    "%1 is needed for this and is not installed.": "Isto precisa de %1, que não está instalado.",
    "%1 can't be converted to %2. It can become: %3.": "%1 não pode ser convertido para %2. Pode virar: %3.",
    "File not found: %1": "Arquivo não encontrado: %1",
    "There is no way to compress %1.": "Não há como comprimir %1.",
    "The conversion failed.": "A conversão falhou.",
    "The conversion stopped unexpectedly.": "A conversão parou inesperadamente.",
    "The conversion engine (lingmo-fileaction-engine) is not installed.":
        "O motor de conversão (lingmo-fileaction-engine) não está instalado.",
    "Version %1": "Versão %1",
    "Convert files by dropping them on a wheel: drag a file, tap Shift, Ctrl or Alt and drop it on a slice.":
        "Converta arquivos soltando-os numa roda: arraste um arquivo, toque Shift, Ctrl ou Alt e solte numa fatia.",
    "Based on FileAction (MIT) by Metamorfolog, inspired by Tangerine.":
        "Baseado no FileAction (MIT) de Metamorfolog, inspirado no Tangerine.",
    # engine: slice names and subtitles
    "Compress": "Comprimir",
    "compressed": "comprimido",
    "Shrink": "Reduzir",
    "Max 1920 px": "Máx. 1920 px",
    "for web": "para a web",
    "Rotate": "Girar",
    "right": "à direita",
    "left": "à esquerda",
    "Mirror": "Espelhar",
    "Square": "Quadrado",
    "crop": "recortar",
    "Grayscale": "Preto e branco",
    "No metadata": "Sem metadados",
    "Merge to PDF": "Juntar em PDF",
    "Merge": "Juntar",
    "document": "documento",
    "video": "vídeo",
    "Normalize": "Normalizar",
    "loudness": "volume",
    "Trim": "Cortar",
    "start–end": "início–fim",
    "Join": "Juntar",
    "audio only": "só o áudio",
    "animation": "animação",
    "downscale": "reduzir",
    "Mute": "Sem som",
    "muted": "mudo",
    "Speed up": "Acelerar",
    "From S-Log": "De S-Log",
    "Sony → normal video": "Sony → vídeo normal",
    "Apply LUT": "Aplicar LUT",
    ".cube file": "arquivo .cube",
    "Thumbnail": "Miniatura",
    "PNG frame": "quadro PNG",
    "all pages": "todas as páginas",
    "all slides": "todos os slides",
    "slides": "slides",
    "text": "texto",
    "text only": "só o texto",
    "tables": "tabelas",
    "web": "web",
    "e-book": "e-book",
    "Merge PDFs": "Juntar PDFs",
    "Split": "Dividir",
    "into pages": "em páginas",
    "Optimize": "Otimizar",
    "Extract": "Extrair",
    "Archive": "Compactar",
    "Extract text": "Extrair texto",
    "Start": "Início",
    "End": "Fim",
    "e.g. 0:30 or 1:02:03": "ex.: 0:30 ou 1:02:03",
    "empty = until the end": "vazio = até o fim",
    "high quality": "alta qualidade",
    "medium quality": "qualidade média",
    "small file": "arquivo pequeno",
    "original size": "tamanho original",
    "same format": "mesmo formato",
    # engine: messages
    "Cancelled.": "Cancelado.",
    "The image is already well compressed; no smaller copy was made.":
        "A imagem já está bem comprimida; nenhuma cópia menor foi feita.",
    "The file is already well compressed; no smaller copy was made.":
        "O arquivo já está bem comprimido; nenhuma cópia menor foi feita.",
    "The image is already smaller than 1920 px; nothing to change.":
        "A imagem já é menor que 1920 px; nada a mudar.",
    "The audio is already at 128 kbit/s or less; smaller would only lose quality.":
        "O áudio já está em 128 kbit/s ou menos; menor só perderia qualidade.",
    "The video is already 720p or smaller; there is nothing to gain.":
        "O vídeo já é 720p ou menor; não há o que ganhar.",
    "The end must come after the start.": "O fim precisa vir depois do início.",
    "The conversion produced no output.": "A conversão não gerou nenhum arquivo.",
    # the settings window (lingmo-fileaction --settings)
    "Settings": "Configurações",
    "File actions settings": "Configurações das ações de arquivo",
    "General": "Geral",
    "Gestures and keys": "Gestos e teclas",
    "Wheels": "Rodas",
    "Quality": "Qualidade",
    "Helper programs": "Programas auxiliares",
    "About": "Sobre",
    "Reset to default": "Restaurar o padrão",
    "How to use": "Como usar",
    "Drag a file and, while dragging, tap a key: %1 converts, %2 opens the tools, %3 the actions. Drop the file on a slice; the result is saved next to the original.":
        "Arraste um arquivo e, enquanto arrasta, toque uma tecla: %1 converte, %2 abre as ferramentas, %3 as ações. Solte o arquivo numa fatia; o resultado é salvo ao lado do original.",
    "In Lingmo Files, right-click a file and choose Convert… or File actions…":
        "No Lingmo Arquivos, clique com o botão direito num arquivo e escolha Converter… ou Ações de arquivo…",
    "%1 opens the wheel for the files selected in Lingmo Files or copied to the clipboard. Arrows choose, Enter runs, Tab switches wheels.":
        "%1 abre a roda para os arquivos selecionados no Lingmo Arquivos ou copiados para a área de transferência. As setas escolhem, Enter executa, Tab troca de roda.",
    "Start with the session": "Iniciar com a sessão",
    "Needed for the drag gesture. Turned off, the wheel still opens from the menu, the shortcut and Lingmo Files. Takes effect at the next login.":
        "Necessário para o gesto de arrastar. Desligado, a roda ainda abre pelo menu, pelo atalho e pelo Lingmo Arquivos. Vale a partir do próximo login.",
    "Where results go": "Onde ficam os resultados",
    "Next to the original, with a new name": "Ao lado do original, com um nome novo",
    "In a folder:": "Numa pasta:",
    "none chosen": "nenhuma escolhida",
    "Choose…": "Escolher…",
    "The original is never changed or overwritten.": "O original nunca é alterado nem sobrescrito.",
    "After finishing": "Ao terminar",
    "Show in the Island": "Mostrar na Ilha",
    "Progress, Cancel, and Open or Show in folder when it is done. Errors are always shown.":
        "Progresso, Cancelar e, ao terminar, Abrir ou Mostrar na pasta. Erros são sempre mostrados.",
    "Open the folder": "Abrir a pasta",
    "Shows the result selected in Lingmo Files.": "Mostra o resultado selecionado no Lingmo Arquivos.",
    "Appearance": "Aparência",
    "Glass wheel": "Roda de vidro",
    "The wheel is translucent and blurs what is behind it. Turned off, it is solid.": "A roda fica translúcida e desfoca o que está atrás dela. Desligada, fica sólida.",
    "Folder for the results": "Pasta para os resultados",
    "Nothing": "Nada",
    "Drag gesture": "Gesto de arrastar",
    "While dragging a file, a quick tap on a key opens a wheel under the pointer. Holding the key (copy or move in a file manager) is not a tap.":
        "Enquanto você arrasta um arquivo, um toque rápido numa tecla abre uma roda sob o ponteiro. Segurar a tecla (copiar ou mover num gerenciador de arquivos) não é um toque.",
    "Tap time": "Tempo do toque",
    "Press and release within this time. Longer is easier, shorter avoids opening the wheel by accident.":
        "Aperte e solte dentro deste tempo. Mais longo é mais fácil; mais curto evita abrir a roda sem querer.",
    "%1 ms": "%1 ms",
    "Which key opens which wheel": "Qual tecla abre qual roda",
    "Tap %1": "Tocar %1",
    "On the wheel, tapping another key switches wheels; the same key again repeats the last action.":
        "Com a roda aberta, tocar outra tecla troca de roda; a mesma tecla de novo repete a última ação.",
    "Keyboard shortcut": "Atalho de teclado",
    "Opens the wheel for the files selected in Lingmo Files, or the files or picture on the clipboard.":
        "Abre a roda para os arquivos selecionados no Lingmo Arquivos, ou para os arquivos ou a imagem na área de transferência.",
    "Press the keys…": "Aperte as teclas…",
    "Back to Meta+Shift+A": "Voltar para Meta+Shift+A",
    "That is not a key combination.": "Isso não é uma combinação de teclas.",
    "%1 is already used by “%2”.": "%1 já é usado por “%2”.",
    "%1 can't be read, so it was not changed.": "Não foi possível ler %1, então ele não foi alterado.",
    "%1 can't be written.": "Não foi possível gravar %1.",
    "Favourites come first, in the order you starred them. Up to %1 slices fit on a wheel.":
        "Os favoritos vêm primeiro, na ordem em que você os marcou. Cabem até %1 fatias numa roda.",
    "Images": "Imagens",
    "Video": "Vídeo",
    "Audio": "Áudio",
    "PDF": "PDF",
    "Documents": "Documentos",
    "Slides": "Apresentações",
    "Spreadsheets": "Planilhas",
    "Subtitles": "Legendas",
    "Archives": "Arquivos compactados",
    "Presets": "Predefinições",
    "All files": "Todos os arquivos",
    "Other": "Outros",
    "“Open with” apps": "Apps de “Abrir com”",
    "The apps for the file's type, your default first.": "Os apps para o tipo do arquivo, o seu padrão primeiro.",
    "Remove from favourites": "Tirar dos favoritos",
    "Favourite: always first": "Favorito: sempre primeiro",
    "Move up": "Mover para cima",
    "Move down": "Mover para baixo",
    "Shown on the wheel": "Aparece na roda",
    "Hidden": "Oculta",
    "Every slice is hidden.": "Todas as fatias estão ocultas.",
    "Preview": "Prévia",
    "The conversion engine sent nothing.": "O mecanismo de conversão não respondeu nada.",
    "lingmo-fileaction-engine is not installed": "o lingmo-fileaction-engine não está instalado",
    "Used by every conversion from now on.": "Vale para todas as conversões a partir de agora.",
    "%1 %": "%1 %",
    "Pictures": "Imagens",
    "JPG, WebP and AVIF quality": "Qualidade de JPG, WebP e AVIF",
    "Converting, rotating, shrinking (Max 1920 px) and PDF pages as pictures.":
        "Converter, girar, reduzir (Máx. 1920 px) e páginas de PDF como imagens.",
    "The Compress slice: lower is smaller.": "A fatia Comprimir: menor valor, arquivo menor.",
    "Remove the location from photos": "Remover a localização das fotos",
    "The GPS position is taken out of converted and edited photos. The “No metadata” slice removes everything.":
        "A posição de GPS é tirada das fotos convertidas e editadas. A fatia “Sem metadados” remove tudo.",
    "PDF pages as pictures": "Páginas de PDF como imagens",
    "Resolution of PDF to JPG and PNG.": "Resolução de PDF para JPG e PNG.",
    "%1 DPI": "%1 DPI",
    "Converting and editing": "Converter e editar",
    "MP4, MOV, MKV, 720p, rotate, trim, speed up.": "MP4, MOV, MKV, 720p, girar, cortar, acelerar.",
    "High (bigger files)": "Alta (arquivos maiores)",
    "Balanced": "Equilibrada",
    "Small": "Pequena",
    "The Compress slice for videos.": "A fatia Comprimir para vídeos.",
    "Light": "Leve",
    "Strong (smallest)": "Forte (menor arquivo)",
    "Stored in FileAction's own settings, so its other apps use them too.":
        "Guardadas nas configurações do próprio FileAction, então os outros apps dele também as usam.",
    "Pictures, audio and video need nothing else. These add more formats.":
        "Imagens, áudio e vídeo não precisam de mais nada. Estes acrescentam mais formatos.",
    "Installed": "Instalado",
    "Not installed": "Não instalado",
    "Dimmed now: %1": "Apagadas agora: %1",
    "Install languages": "Instalar idiomas",
    "Install": "Instalar",
    "Installed with the system's package manager; your password is asked.":
        "Instalados pelo gerenciador de pacotes do sistema; sua senha é pedida.",
    "Check again": "Verificar de novo",
    "Word, Excel and PowerPoint files: DOCX, XLSX, PPTX, ODT, ODS and ODP to PDF and back, PDF to PPTX.":
        "Arquivos do Word, Excel e PowerPoint: DOCX, XLSX, PPTX, ODT, ODS e ODP para PDF e de volta, PDF para PPTX.",
    "Documents: Markdown, HTML, EPUB, RTF, ODT and DOCX to each other.":
        "Documentos: Markdown, HTML, EPUB, RTF, ODT e DOCX entre si.",
    "Extract text (OCR) from pictures and scans.": "Extrair texto (OCR) de imagens e digitalizações.",
    "Extract text (Images)": "Extrair texto (Imagens)",
    "No reading languages installed": "Nenhum idioma de leitura instalado",
    "Languages: %1": "Idiomas: %1",
    "Markdown and HTML to PDF, with the page as a browser prints it.":
        "Markdown e HTML para PDF, com a página como um navegador a imprime.",
    "Waiting for authorization…": "Aguardando autorização…",
    "Downloading…": "Baixando…",
    "Installing…": "Instalando…",
    "%1 is installed.": "%1 está instalado.",
    "The installation was cancelled.": "A instalação foi cancelada.",
    "Another installation is running. Try again when it finishes.":
        "Outra instalação está em andamento. Tente de novo quando ela terminar.",
    "%1 could not be installed. Check the internet connection.":
        "Não foi possível instalar %1. Verifique a conexão com a internet.",
    "Could not start pkexec. Make sure polkit is installed.":
        "Não foi possível iniciar o pkexec. Verifique se o polkit está instalado.",
    "engine %1": "mecanismo %1",
    "File actions is free software under the GPL 3.0 or later. The conversions are FileAction's code, under the MIT licence.":
        "Ações de arquivo é software livre sob a GPL 3.0 ou posterior. As conversões são código do FileAction, sob a licença MIT.",
    "File actions (this app)": "Ações de arquivo (este app)",
    "The conversion engine (FileAction fork)": "O mecanismo de conversão (fork do FileAction)",
    "FileAction by Metamorfolog": "FileAction, de Metamorfolog",
    "Licences": "Licenças",
}


def main(path):
    tree = ET.parse(path)
    missing = []
    for m in tree.iter("message"):
        src = m.find("source").text or ""
        tr = m.find("translation")
        if src in PT:
            text = PT[src]
        else:
            missing.append(src)
            continue
        if isinstance(text, list):  # numerus: one form per plural
            for child in list(tr):
                tr.remove(child)
            tr.text = None
            for form in text:
                ET.SubElement(tr, "numerusform").text = form
        else:
            tr.text = text
        tr.attrib.pop("type", None)
    tree.write(path, encoding="utf-8", xml_declaration=True)
    for s in missing:
        print("missing:", s)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
