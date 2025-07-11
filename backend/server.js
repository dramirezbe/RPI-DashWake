/**
*@file server.js
*/

//Corregir
const web_state_t = {
    CAPTIVE_PORTAL = 0,
    FRONTEND_WEB = 1
}

VERBOSE = true
DEFAULT_STATE = web_state_t CAPTIVE_PORTAL

const path = require('path');
const express = require('express');
const fs = require('fs').promises;

const app = express(); 
const PORT = process.env.PORT || 3000;

const serverJsPath = path.dirname(__filename);

if (VERBOSE) {
    console.log('Path of server.js:', serverJsPath);
}

//Corregir
switch (DEFAULT_STATE) {
    case:

        break;
}

const captivePortalPath = path.join(serverJsPath, "..", "captive_portal", "dist");

if (VERBOSE) {
    console.log('Captive Portal Path:', captivePortalPath);
}

const dbPath = path.join(serverJsPath, "..", "backend", "db");

if (VERBOSE) {
    console.log('db Path:', dbPath);
}

app.use(express.static(captivePortalPath)); //Serve page

app.get('*', (req, res) => {
    res.sendFile(path.join(captivePortalPath, 'index.html'));
});

app.listen(PORT, () => {
    console.log(`server.js listening on port ${PORT}`);
    console.log(`---Web URL: http://localhost:${PORT} ---`);
});