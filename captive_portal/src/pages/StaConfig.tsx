/**
 * @file StaConfig.tsx
 * @description Página de configuración para el modo Estación Wi-Fi.
 */

import React from 'react';
import './StaConfig.css';

const StaConfig: React.FC = () => {
  return (
    <div className="sta-config-container">
      <div className="sta-config-card">
        <h2 className="sta-config-title">Configuración de Station Wi-Fi</h2>
        <p className="sta-config-description">
          Aquí podrás configurar tu dispositivo para conectarse a una red Wi-Fi existente como cliente.
        </p>
        <div className="sta-config-actions">
          {/* Puedes añadir formularios o elementos de configuración aquí */}
          <button className="sta-config-button">
            Conectar a Red
          </button>
        </div>
      </div>
    </div>
  );
};

export default StaConfig;
