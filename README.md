Custom C++ Web Browser Engine

A fully autonomous, lightweight web browser engine built entirely from scratch in C++. This project bypasses standard browser frameworks (like Chromium or WebKit) to implement custom raw network socket handling, a state-machine DOM parser, and a GPU-accelerated layout engine.

ore Architecture

* **Network Stack (ASIO & OpenSSL):** Custom TCP/TLS socket implementation capable of secure HTTPS handshakes, 3xx redirect tracing, and mathematical decoding of HTTP/1.1 Chunked Transfer Encoding.
* **HTML/CSS DOM Parser:** A custom state-machine parser that ingests raw HTML strings, dynamically extracts attributes (`href`, `src`), translates both decimal and hexadecimal Unicode entities into UTF-8, and builds a traversable DOM tree. Includes a tokenizer to parse inline `<style>` blocks for dynamic rendering overrides.
* **Layout & Render Engine (SFML):** Utilizes hardware-accelerated graphics to render the DOM. Implements a dual-camera system (static UI view vs. scrolling document view) and dynamic text wrapping.
* **Multimedia & Asynchronous Fetching:** Dynamically identifies `<img src="...">` tags, fires secondary ASIO network requests to download binary image bytes, and loads them directly into GPU memory via `sf::Texture`.
* **Interactive UI & Raycasting:** Features a persistent address bar, a localized Last-In-First-Out (LIFO) history stack for the Back button, and mathematical raycasting to detect mouse collisions with hyperlinks.
* **Web Forms Engine:** Maps `<form>` and `<input>` tags to interactive screen space. Captures keyboard focus, aggregates user input, and generates URL-encoded query strings for live GET request submissions.