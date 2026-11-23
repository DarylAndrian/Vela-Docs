// ════════════════════════════════════════════════════════════════════════════
//  vela-bridge.js — JavaScript client for the C++ EditorBridge
//
//  The Qt shell registers `EditorBridge` on the page's QWebChannel under the
//  name "vela" (see MainWindow::setupWebChannel). This module turns that raw
//  object into a promise-based API the editor can actually use:
//
//      const { ok, body } = await velaBridge.invoke('getBody');
//
//  In a plain browser (`npm run dev`) there is no QWebChannel, so every call
//  resolves with { ok:false, error:'native bridge unavailable' } instead of
//  hanging — the playground keeps working without Qt.
//
//  ── Why the buffering below is necessary ─────────────────────────────────
//  EditorBridge::invokeCommand() resolves synchronously: it emits
//  commandResult(requestId, …) *before* it returns. QWebChannel therefore
//  delivers the result signal ahead of the method's return-value callback, so
//  at the moment the result arrives JS does not yet know which requestId it
//  asked for. Results are parked by requestId until the callback supplies it.
// ════════════════════════════════════════════════════════════════════════════

const CALL_TIMEOUT_MS = 8000;

class VelaBridge {
    constructor() {
        this.isNative = false;
        this._obj = null;
        /** requestId -> resolve, once we know the id we asked for. */
        this._waiters = new Map();
        /** requestId -> result, for results that beat the callback. */
        this._early = new Map();
        this._connectPromise = null;
    }

    /** Resolves once the channel is up, or immediately with isNative=false. */
    connect() {
        if (this._connectPromise) return this._connectPromise;

        this._connectPromise = new Promise((resolve) => {
            // QWebEngine only injects qt.webChannelTransport when a QWebChannel
            // has been set on the page. qwebchannel.js provides the QWebChannel
            // constructor and is loaded by src/editor/index.html.
            const transport = window.qt && window.qt.webChannelTransport;
            if (!transport || typeof QWebChannel === 'undefined') {
                console.info('[vela-bridge] no QWebChannel — running outside Qt');
                resolve(this);
                return;
            }

            try {
                /* global QWebChannel */
                new QWebChannel(transport, (channel) => {
                    const obj = channel.objects && channel.objects.vela;
                    if (!obj) {
                        console.warn('[vela-bridge] channel has no "vela" object');
                        resolve(this);
                        return;
                    }
                    this._obj = obj;
                    this.isNative = true;

                    obj.commandResult.connect((requestId, ok, payload) => {
                        this._onResult(Number(requestId), !!ok, payload || {});
                    });
                    if (obj.commandTimeout) {
                        obj.commandTimeout.connect((requestId, command) => {
                            console.warn(`[vela-bridge] C++ timeout for "${command}" (id ${requestId})`);
                        });
                    }
                    if (obj.errorReported) {
                        obj.errorReported.connect((where, message) => {
                            console.error(`[vela-bridge] ${where}: ${message}`);
                        });
                    }

                    console.info('[vela-bridge] connected to native EditorBridge');
                    resolve(this);
                });
            } catch (err) {
                console.error('[vela-bridge] QWebChannel setup failed:', err);
                resolve(this);
            }
        });
        return this._connectPromise;
    }

    _onResult(requestId, ok, payload) {
        const waiter = this._waiters.get(requestId);
        if (waiter) {
            this._waiters.delete(requestId);
            waiter.result({ ok, ...payload });
        } else {
            // Arrived before invokeCommand's callback told us the id.
            this._early.set(requestId, { result: { ok, ...payload } });
        }
    }

    /**
     * Send a command to the C++ side.
     * @param {string} command  e.g. 'getBody', 'saveDocument', 'getStats'
     * @param {object} [args]
     * @returns {Promise<object>} the payload, always carrying an `ok` flag
     */
    invoke(command, args = {}) {
        if (!this.isNative || !this._obj) {
            return Promise.resolve({ ok: false, error: 'native bridge unavailable' });
        }

        return new Promise((resolve) => {
            let settled = false;
            const finish = (value) => {
                if (settled) return;
                settled = true;
                clearTimeout(timer);
                resolve(value);
            };

            const timer = setTimeout(() => {
                finish({ ok: false, error: `bridge call "${command}" timed out` });
            }, CALL_TIMEOUT_MS);

            try {
                this._obj.invokeCommand(command, args, (requestId) => {
                    const id = Number(requestId);
                    // The result may already be parked for this id.
                    const early = this._early.get(id);
                    if (early) {
                        this._early.delete(id);
                        finish(early.result);
                        return;
                    }
                    this._waiters.set(id, { result: finish });
                });
            } catch (err) {
                finish({ ok: false, error: `invokeCommand threw: ${err && err.message}` });
            }
        });
    }

    // ── Convenience wrappers ────────────────────────────────────────────────
    getBody()          { return this.invoke('getBody'); }
    setBody(body, documentId) { return this.invoke('setBody', { body, documentId }); }
    onActiveDocumentChanged(listener) { this._obj.activeDocumentChanged.connect(listener); }
    getStats()         { return this.invoke('getStats'); }
    getPlainText()     { return this.invoke('getPlainText'); }
    saveDocument(path) { return this.invoke('saveDocument', path ? { path } : {}); }
    saveDocumentAs(p)  { return this.invoke('saveDocumentAs', { path: p }); }
    openDocument(path) { return this.invoke('openDocument', { path }); }
    newDocument()      { return this.invoke('newDocument'); }
    reportError(where, message) {
        // Fire-and-forget diagnostic channel into the C++ log.
        return this.invoke('reportError', { where, message });
    }
    /// Called on every edit, so it skips the requestId round-trip and hits the
    /// C++ slot directly; the native status bar is the only consumer.
    reportStats(words, characters, pages) {
        if (!this.isNative || !this._obj || typeof this._obj.reportStats !== 'function') return;
        this._obj.reportStats(words, characters, pages);
    }
}

export const velaBridge = new VelaBridge();

// Exposed for the Qt shell and for debugging from the console.
window.velaBridge = velaBridge;
