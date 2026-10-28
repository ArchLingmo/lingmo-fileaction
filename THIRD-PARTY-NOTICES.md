# Third-party software

**Ações de arquivo** (lingmo-fileaction) is GPL-3.0-or-later (see [LICENSE](LICENSE)). It is based on
**FileAction** by Metamorfolog (https://github.com/Metamorfolog/file_action), inspired by
[Tangerine](https://tangerineformac.com/) for macOS (FileAction is not affiliated with Tangerine, and neither is Lingmo).

- The conversions run in `lingmo-fileaction-engine`, our fork of FileAction
  (https://github.com/ArchLingmo/lingmo-fileaction-engine), installed as a separate program and called on the
  command line. Its license and the notices of the libraries it bundles (Magick.NET/ImageMagick, PDFtoImage/PDFium/
  SkiaSharp, PdfPig, PDFsharp, ClosedXML, ExcelDataReader, SharpCompress) are in
  `/usr/share/licenses/lingmo-fileaction-engine/`.
- The idea and behaviour of the wheel (three wheels on Shift, Ctrl and Alt, drop in the middle to cancel, the same
  key repeats the last action, results next to the original) follow FileAction. The QML wheel and the X11 gesture
  are new code written for Lingmo.
- FFmpeg (GPL), Pandoc (GPL) and LibreOffice (MPL-2.0) are separate programs from the Arch repositories.

## FileAction

MIT License

Copyright (c) 2026 Metamorfolog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
