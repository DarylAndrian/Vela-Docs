# Spell-check dictionaries

`en-US.bdic` is the [Chromium English (US) dictionary](https://chromium.googlesource.com/chromium/deps/hunspell_dictionaries/+/refs/heads/main/en-US-10-2.bdic), distributed here with its upstream notices. SHA-256 `b0d80e6b6c5491879365e176ecbd98074602a663429bcfcfb0a5a8375c051f13`.

The Windows build copies the dictionary into `qtwebengine_dictionaries` beside the executable. Vela Docs uses Qt WebEngine's spelling suggestions and works offline with the bundled dictionary.
