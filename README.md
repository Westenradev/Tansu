# Tansu

**Tansu** is a small, free, open-source **native app for Haiku OS** — a floating
shelf window where you temporarily park files, text snippets and images during
drag-and-drop, so you can navigate freely between windows, folders and
workspaces before dropping the items at their final destination.

Keywords: `haiku` `haiku-os` `beos` `haiku-app` `native` `cpp` `drag-and-drop`
`file-utility` `shelf` `open-source` `community`

**PRs welcome — anyone can contribute.**

> ⚠️ **Disclaimer**
>
> 1. This is a **first-time project** by the author — a learning project.
> 2. It is **experimental and meant as a test** — provided as-is, with no
>    warranty of any kind.
> 3. The code was developed **with AI assistance**.
> 4. **Use at your own risk.**

## Features

Tested on Haiku R1/beta6 (x86_64).

- Floating shelf window at the screen edge, always on top, resizable
  vertically; it remembers its position and height.
- Drop **files and folders** (from Tracker), **text** (from StyledEdit,
  Terminal, mail and other text views) and **images** (for example a
  selection dragged from ShowImage) onto the shelf.
- One row per item: real Tracker icons for files, a one-line snippet for
  text, a thumbnail for images.
- Drag items back out: files to Tracker or any app that accepts files, text
  to text views (or to Tracker, where it becomes a text clipping), images to
  Tracker (saved as a PNG file). Select several rows (Shift-click) to drag
  them together.
- Right-click a row: **Open**, **Show in Tracker**, **Remove**. Right-click
  anywhere: **Paste from clipboard**, **Clear shelf**, **About Tansu**,
  **Quit**. Double-click a row to open it.
- Files on the shelf are followed when they are renamed or moved, and
  removed from the shelf when they are deleted.
- The shelf (files, text and images) survives restarts; it is stored in
  `~/config/settings/Tansu/`.
- Optional Tracker add-on: right-click files → **Add-ons → Add to Tansu**
  (starts Tansu if it is not running). You can also drop files on the Tansu
  icon or run `Tansu file1 file2 ...`.

When files are dragged from the shelf into a Tracker window, Tracker copies
them; the originals stay where they are.

## Building

On Haiku (x86_64), inside a Terminal:

```bash
pkgman install -y cmd:git cmd:cmake cmd:ninja cmd:g++ haiku_devel
```

```bash
git clone https://github.com/Westenradev/Tansu.git && cd Tansu
cmake -B build -G Ninja && ninja -C build && ./build/Tansu
```

The Tracker add-on is built as `build/Add to Tansu`; install it with:

```bash
mkdir -p ~/config/non-packaged/add-ons/Tracker
cp "build/Add to Tansu" ~/config/non-packaged/add-ons/Tracker/
```

It appears in Tracker's **Add-ons** menu right away. (`~/config/add-ons` is
managed by the package system and is read-only.)

The app icon is drawn in `resources/icon.svg` (it follows Haiku's
[icon guidelines](https://www.haiku-os.org/development/icon-guidelines) and can
be opened in Icon-O-Matic or Inkscape). `tools/make-icon.py` converts it to
Haiku's vector icon format and updates the `vector_icon` resource in
`resources/app.rdef`:

```bash
python3 tools/make-icon.py
```

## Contributing

This project is a gift to the Haiku community — **anyone can contribute**,
and issues and pull requests are welcome. If you are an experienced Haiku
developer, please feel free to mentor or correct beginner mistakes; that is
part of the point of this project.

## License

MIT License — see [LICENSE](LICENSE). Copyright (c) 2026 Westenra Softworks.

## Future ideas

- Archivable shelf replicant for the Desktop (BDragger / BShelf).
- Auto-start on login (add to `~/config/settings/boot/launch`).
- Keyboard shortcuts (the shelf never takes focus, so it has none yet).
- Translations (Haiku's locale kit).
