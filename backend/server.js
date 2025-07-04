/**
*@file server.js
*/

VERBOSE = true

const path = require('path');
const express = require('express');
const fs = require('fs').promises;

const app = express(); 
const PORT = process.env.PORT || 3000;

const serverJsPath = path.dirname(__filename);

if (VERBOSE) {
    console.log('Path of server.js:', serverJsPath);
}


const captivePortalPath = path.join(serverJsPath, "..", "captive_portal", "dist");

if (VERBOSE) {
    console.log('Captive Portal Path:', captivePortalPath);
}
const usersJsonPath = path.join(serverJsPath, "..", "backend", "db", "users.json");

if (VERBOSE) {
    console.log('users.json Path:', usersJsonPath);
}

app.use(express.static(captivePortalPath)); //Serve page

app.listen(PORT, () => {
    console.log(`Server listening on port ${PORT}`);
    console.log(`--------------------- http://localhost:${PORT} ---------------------`);
});