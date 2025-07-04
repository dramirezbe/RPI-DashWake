/**
*@file App.tsx
*/

import { Routes, Route } from 'react-router-dom';
import LoginPage from './pages/LoginPage';
import './App.css';

function App() {
  return (
    <div className="App">
      <Routes>
        {/* Route for the Login Page */}
        <Route path="/login" element={<LoginPage />} />

        {/* You can add other routes here, e.g., a home page or a dashboard */}
        <Route path="/" element={<div><h1>Welcome!</h1><p>Navigate to /login to see the login page.</p></div>} />
      </Routes>
    </div>
  );
}

export default App;