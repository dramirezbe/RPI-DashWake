/**
 * @file APConfig.tsx
 * @description Página de configuración para el modo Access Point.
 */

import React from 'react';
import './APConfig.css'; // Import the CSS file

const APConfig: React.FC = () => {
  return (
    <div className="ap-config-container">
      <div className="ap-config-card">
        <h2 className="ap-config-title">Configuración de Access Point</h2>
        <p className="ap-config-description">
          Aquí podrás configurar los ajustes de tu dispositivo cuando funcione como un punto de acceso Wi-Fi.
        </p>
        <div className="ap-config-actions">
          {/* Puedes añadir formularios o elementos de configuración aquí */}
          <button className="ap-config-button">
            Guardar Configuración
          </button>
        </div>
      </div>
    </div>
  );
};

export default APConfig;
