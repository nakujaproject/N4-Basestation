import React from 'react';
import logo from '../assets/nakujaLogo.png'

function Footer() {
  // Get the current year dynamically
  const currentYear = new Date().getFullYear();

  return (
    <footer className=" bg-gray-200 text-gray-700 py-4">
      <div className="container  mx-auto flex flex-col md:flex-row items-center justify-center">
        <div className="mt-2 md:mt-0">
          <p className="text-lg text-center">
            &copy; {currentYear} Nakuja.
          </p>
        </div>
      </div>
    </footer>
  );
};

export default Footer;
