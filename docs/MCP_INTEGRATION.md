# MCP Server Integration — Vela Docs

> How to wire your Model Context Protocol (MCP) server into Cline (the AI
> coding assistant you're using right now) so you can call it during
> development of this project.

---

## 1. What is an MCP server?

An MCP server is a process that exposes **tools** (and **resources** and
**prompts**) over a standard protocol. Cline speaks MCP, so once a server is
configured, its tools become available to you in chat as if they were
built-in — e.g. you can ask Cline to *"use the Stitch tool to generate a
button component"* and it'll route the call to your server.

Two transports are supported:
- **stdio** — local subprocess (most common for dev tools)
- **HTTP / SSE** — remote or local HTTP server

---

## 2. Where the config lives

Cline reads MCP servers from `cline_mcp_settings.json`, which you can edit
directly or via the **MCP Servers** panel in the Cline UI.

### Location on Windows
```
%APPDATA%\Code\User\globalStorage\saoudrizwan.claude-dev\settings\cline_mcp_settings.json
```

In Explorer, paste this into the address bar:
```
%APPDATA%\Code\User\globalStorage\saoudrizwan.claude-dev\settings
```

### Or open from inside VS Code
1. Open the Cline sidebar (the robot icon in the activity bar)
2. Click **MCP Servers** at the top
3. Click **Edit MCP Settings** or **Install Server**

---

## 3. Config format

```jsonc
{
  "mcpServers": {
    // Each key is the server's display name in Cline's UI
    "my-server-name": {
      // ─── stdio transport (local subprocess) ─────────────────────
      "command": "node",                 // or "python", "uvx", etc.
      "args": ["/path/to/server.js"],    // CLI args for the server
      "env": {                           // optional env vars
        "API_KEY": "..."
      },
      "disabled": false,                 // set true to keep installed but disabled
      "autoApprove": []                  // optional: tool names that don't need user approval
    },

    // ─── HTTP / SSE transport ────────────────────────────────────
    "remote-server": {
      "url": "http://localhost:3000/mcp",
      "headers": {
        "Authorization": "Bearer YOUR_TOKEN"
      }
    }
  }
}
```

---

## 4. Example: Google Stitch MCP

If you've built a Stitch MCP wrapper (or are using a community one), the
config might look like:

### Option A — stdio (Node-based server)

```json
{
  "mcpServers": {
    "stitch": {
      "command": "node",
      "args": ["C:/tools/stitch-mcp/index.js"],
      "env": {
        "STITCH_API_KEY": "your-key-from-google-stitch-console",
        "STITCH_PROJECT_ID": "vela-docs"
      }
    }
  }
}
```

### Option B — stdio via `npx` (no local install)

```json
{
  "mcpServers": {
    "stitch": {
      "command": "npx",
      "args": ["-y", "@your-org/stitch-mcp"],
      "env": {
        "STITCH_API_KEY": "your-key-from-google-stitch-console"
      }
    }
  }
}
```

### Option C — Python with `uv`

```json
{
  "mcpServers": {
    "stitch": {
      "command": "uv",
      "args": [
        "tool", "run",
        "--from", "stitch-mcp",
        "stitch-mcp-server"
      ],
      "env": {
        "STITCH_API_KEY": "your-key-from-google-stitch-console"
      }
    }
  }
}
```

### Option D — Remote HTTP server (if Stitch hosts one)

```json
{
  "mcpServers": {
    "stitch": {
      "url": "https://mcp.stitch.withgoogle.com/v1",
      "headers": {
        "Authorization": "Bearer your-key-from-google-stitch-console"
      }
    }
  }
}
```

---

## 5. Verify the server is connected

1. Save `cline_mcp_settings.json`
2. **Reload VS Code** (`Ctrl+Shift+P` → "Developer: Reload Window")
   — required for Cline to pick up new servers.
3. Open the Cline sidebar → **MCP Servers**.
4. Your server should appear with a green dot (connected) or red (error).
5. Hover for status; click to see logs.

In chat, the tools will be available as:
```
use_mcp_tool tool: 'stitch.generate_ui' arguments: {...}
```

You can also use `access_mcp_resource` to fetch files/data the server exposes.

---

## 6. Calling your MCP server from Cline

Once connected, ask Cline in plain English:

> *"Use the Stitch MCP to generate a UI for the ribbon toolbar based on
> the design prompt for this project."*

Or be explicit:

> *"Call mcp__stitch__generate_ui with a prompt describing the
> ribbon toolbar design and write the output to src/editor/index.html."*

Cline will automatically pick the right tool from your server's tool list.

---

## 7. MCP servers useful for Vela Docs

| Server                | What it does                              | Transport | Notes |
|-----------------------|-------------------------------------------|-----------|-------|
| **Stitch**            | UI design generation                     | stdio / HTTP | Generates HTML/CSS/JS from a design prompt |
| **filesystem**        | Sandboxed filesystem access               | stdio     | Pre-built — restrict to project dir |
| **git**               | Git history & diffs                       | stdio     | Pre-built |
| **github**            | PR / Issue management                     | HTTP      | Auth via PAT |
| **postgres / sqlite** | Database inspection                       | stdio     | Useful for debugging the revision DB |
| **puppeteer**         | Headless browser for visual regression    | stdio     | Good for Phase 6 testing |

The **filesystem**, **git**, and **github** servers come pre-installed and
just need to be enabled in `cline_mcp_settings.json`.

---

## 8. Troubleshooting

### "Server disconnected" or red dot
- Check the Cline → MCP Servers panel for error logs.
- Most common cause: wrong path or missing dependency.
- Test the server standalone in a terminal first:
  ```bash
  node /path/to/server.js
  ```
  You should see it print a "ready" message on stdin/stdout.

### Tools don't appear in chat
- After editing `cline_mcp_settings.json`, **reload the window**.
- Make sure `"disabled": false` (or omit the key).

### Windows path issues
- Use forward slashes: `"C:/tools/stitch-mcp/index.js"`
- Or escaped backslashes: `"C:\\tools\\stitch-mcp\\index.js"`
- Don't use single backslashes — JSON will choke.

### Environment variables
- Set them in the `"env"` block of the server entry, NOT in your shell.
  Cline launches the subprocess with a controlled environment.

### Server needs `npx` or `uvx`
- These must be installed on your system and on `PATH`.
- `npx` ships with Node.js. Install Node.js from https://nodejs.org.
- `uvx` ships with [uv](https://docs.astral.sh/uv/).

---

## 9. Project-local MCP config (recommended)

For team-shared MCP setup, drop a `mcp.json` in the project root:

```jsonc
// mcp.json — version-controlled example, not auto-loaded by Cline
{
  "mcpServers": {
    "stitch": {
      "command": "npx",
      "args": ["-y", "@your-org/stitch-mcp"],
      "env": {
        "STITCH_API_KEY": "${env:STITCH_API_KEY}"
      }
    }
  }
}
```

Then in your personal `cline_mcp_settings.json`:

```json
{
  "mcpServers": {
    "stitch": {
      "command": "npx",
      "args": ["-y", "@your-org/stitch-mcp"],
      "env": {
        "STITCH_API_KEY": "paste-your-key-here"
      }
    }
  }
}
```

Keeps secrets out of git, configuration in sync.

---

## 10. Next steps

1. Configure your Stitch MCP server (or any other) in
   `cline_mcp_settings.json`.
2. Reload VS Code.
3. Try asking: *"List the tools available from the stitch MCP server."*
4. Then: *"Generate a Qt ribbon toolbar design using the Stitch MCP."*

Cline will then call your Stitch server and drop the resulting HTML/CSS
into `src/editor/` for you to integrate.
