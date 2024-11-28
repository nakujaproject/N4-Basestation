import React from "react";

function Button ({ onClick, children, className }){
  const handleRipple = (e) => {
    const button = e.currentTarget;

    // Create the ripple element
    const ripple = document.createElement("span");
    ripple.className = "ripple";

    // Set the position and size of the ripple
    const rect = button.getBoundingClientRect();
    const size = Math.max(rect.width, rect.height);
    ripple.style.width = ripple.style.height = `${size}px`;

    const x = e.clientX - rect.left - size / 2;
    const y = e.clientY - rect.top - size / 2;
    ripple.style.left = `${x}px`;
    ripple.style.top = `${y}px`;

    // Append the ripple to the button and remove it after animation
    button.appendChild(ripple);
    setTimeout(() => {
      ripple.remove();
    }, 600);
  };

  const handleClick = (e) => {
    handleRipple(e);
    if (onClick) onClick(e);
  };

  return (
    <button
      onClick={handleClick}
      className={`relative overflow-hidden ${className}`}
    >
      {children}
    </button>
  );
};

export default Button;
