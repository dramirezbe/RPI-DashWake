/**
 * @file App.tsx
 * @description Configuración principal de la aplicación con enrutamiento y encabezado.
 */

import { Routes, Route, Navigate } from 'react-router-dom';

import GeneraHeader from './assets/GeneralHeader'; // Asegúrate de que la ruta sea correcta

import LoginPage from './pages/LoginPage';
import APConfig from './pages/APConfig';
import StaConfig from './pages/StaConfig';

import './App.css';

function App() {
  return (
    <div className="App">
      <GeneraHeader />

      <Routes>
        <Route path="/" element={<Navigate to="/login" replace />} />
        <Route path="/login" element={<LoginPage />} />
        <Route path="/station-config" element={<StaConfig />} />
        <Route path="/ap-config" element={<APConfig />} />
      </Routes>
    </div>
  );
}

export default App;
