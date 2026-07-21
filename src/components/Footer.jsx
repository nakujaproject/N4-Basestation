import React from "react";
import { Github, Linkedin, Mail, Globe } from "lucide-react";
import logo from "../assets/nakujaLogo.png";

function Footer() {
  const currentYear = new Date().getFullYear();

  return (
    <footer className="bg-gray-100 text-gray-800 border-t border-gray-300 py-10">
      <div className="container mx-auto px-6 md:px-16">
        {/* Top Section */}
        <div className="flex flex-col md:flex-row justify-between items-center md:items-start gap-8">
          {/* Logo & About */}
          <div className="flex flex-col items-center md:items-start text-center md:text-left space-y-3">
            <img
              src={logo}
              alt="Nakuja Logo"
              className="w-16 h-16 object-contain"
            />
            <h2 className="text-xl font-semibold">Nakuja</h2>
            <p className="text-sm text-gray-600 max-w-sm">
              Empowering innovation and sustainability through technology,
              research, and collaboration.
            </p>
          </div>

          {/* Navigation Links */}
          <div className="grid grid-cols-2 md:grid-cols-3 gap-6 text-center md:text-left">
            <div>
              <h3 className="font-semibold mb-2">Explore</h3>
              <ul className="space-y-1 text-sm">
                <li><a href="https://nakujaproject.com/" className="hover:text-blue-600">Home</a></li>
                <li><a href="https://nakujaproject.com/about.html" className="hover:text-blue-600">About</a></li>
                <li><a href="https://nakujaproject.com/research.html" className="hover:text-blue-600">Projects</a></li>
                <li><a href="https://nakujaproject.com/team.html" className="hover:text-blue-600">Team</a></li>
              </ul>
            </div>
            <div>
              <h3 className="font-semibold mb-2">Resources</h3>
              <ul className="space-y-1 text-sm">
                <li><a href="https://nakujaproject.blogspot.com/" className="hover:text-blue-600">Blog</a></li>
                <li><a href="https://nakujaproject.com/join.html" className="hover:text-blue-600">Join</a></li>
                {/* <li><a href="#gallery" className="hover:text-blue-600">Gallery</a></li> */}
              </ul>
            </div>
            <div>
              <h3 className="font-semibold mb-2">Connect</h3>
              <ul className="space-y-1 text-sm">
                <li><a href="#contact" className="hover:text-blue-600">Contact Us</a></li>
                <li><a href="mailto:info@nakuja.org" className="hover:text-blue-600">Email</a></li>
                <li><a href="https://nakujaproject.com" target="_blank" className="hover:text-blue-600">Website</a></li>
              </ul>
            </div>
          </div>

          {/* Social Links */}
          <div className="flex space-x-4 text-gray-600">
            <a
              href="https://github.com/nakuja"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-blue-600"
            >
              <Github size={22} />
            </a>
            <a
              href="https://linkedin.com/company/nakuja"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-blue-600"
            >
              <Linkedin size={22} />
            </a>
            <a
              href="mailto:info@nakuja.org"
              className="hover:text-blue-600"
            >
              <Mail size={22} />
            </a>
            <a
              href="https://nakuja.org"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-blue-600"
            >
              <Globe size={22} />
            </a>
          </div>
        </div>

        {/* Bottom Section */}
        <div className="border-t border-gray-300 mt-10 pt-4 text-center text-sm text-gray-500">
          <p>
            &copy; {currentYear} Nakuja. All rights reserved.
          </p>
        </div>
      </div>
    </footer>
  );
}

export default Footer;
