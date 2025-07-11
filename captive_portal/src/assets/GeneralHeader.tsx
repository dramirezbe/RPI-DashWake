/**
 * @file GeneralHeader.tsx
 * @description Componente de encabezado para la navegación de la aplicación.
 */

import React from 'react';
import { Link } from 'react-router-dom';
import './GeneralHeader.css'; // ¡Importamos el CSS aquí!

const GeneralHeader: React.FC = () => {
  return (
    <header className="header">
      {/* Logo SVG a la izquierda */}
      <div className="header-logo-container">
        <img src="/logo.png" alt="Mi App Logo" className="header-logo-svg" />
      </div>

      {/* Opciones de navegación a la derecha */}
      <nav className="header-nav">
        <div className="header-nav-links">
          <Link to="/station-config" className="header-nav-link">
            Station Wifi Config
          </Link>
          <Link to="/ap-config" className="header-nav-link">
            AccessPoint Wifi Config
          </Link>
          <Link to="/login" className="header-nav-link header-nav-link-last">
            Login Page
          </Link>
        </div>
      </nav>
    </header>
  );
};

export default GeneralHeader;
