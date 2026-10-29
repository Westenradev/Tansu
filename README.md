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

## Status

Early skeleton (milestone M0). What works today:

- Floating shelf window pinned to a screen edge, always on top, resizable
  vertically.
- Highlights when something is dragged over it.
- Remembers its position and height in `~/config/settings/Tansu/settings`.
- App icon.

## Planned features

Not implemented yet:

- Accepts drops of files/folders, plain text and images.
- One row per item: real file icons, one-line text snippets, image thumbnails.
- Drag items back out into any Haiku app (Tracker windows, text editors,
  mail composers); multi-select and drag several rows at once.
- Context menus: Open, Show in Tracker, Remove per row; Clear shelf, Paste
  from clipboard, About, Quit for the shelf itself.
- Shelf contents survive restarts (file items) via `~/config/settings/Tansu/`.
- Optional Tracker add-on: right-click a file → **Add-ons → Add to Tansu**.

## Building

On Haiku (x86_64), inside a Terminal:

```bash
pkgman install -y cmd:git cmd:cmake cmd:ninja cmd:g++ haiku_devel
```

```bash
git clone https://github.com/Westenradev/Tansu-for-Haiku_os.git && cd Tansu-for-Haiku_os
cmake -B build -G Ninja && ninja -C build && ./build/Tansu
```

The Tracker add-on is built as `build/Add to Tansu.so` (it launches Tansu,
but the app does not add the files to the shelf yet); install it with:

```bash
mkdir -p ~/config/add-ons/Tracker
cp "build/Add to Tansu.so" ~/config/add-ons/Tracker/
# then restart Tracker
```

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
- Session persistence for text and image items.
- Auto-start on login (add to `~/config/settings/boot/launch`).
