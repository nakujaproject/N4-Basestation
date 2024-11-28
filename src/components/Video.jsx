import React, { useEffect, useState } from 'react';
import gsap from 'gsap';
import logo from "../assets/nakuja_logo.png";


const Video = () => {
    const [stream, setStream] = useState(false);

    const animate = () => {
        gsap.to('.animate', {
            rotation: '+=360',
            scale: 0.8,
            repeat: -1,
            yoyo: true,
            duration: 2,
            ease: 'power2.inOut',
        });
    };

    useEffect(() => {
        animate();

        // setStream(true); // Assuming the stream is always available from the server

        return () => {
            setStream(false);
        };
    }, [stream]);

    return (
        <div className="shadow-md h-[315px] w-full md:h-full lg:h-full bg-black flex justify-center items-center aspect-video">
            {stream ? (
                
                <iframe 
                    className="text-white w-full h-full" 
                    src={'http://' + import.meta.env.VITE_VIDEO_URL   + '/stream.mjpg'} alt="streaming..." 
                    allowFullScreen
                    allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" 

                    />

            ) :
            (
                <div className='animate'>
                    <img
                        alt="logo"
                        src={logo}
                        width="80"
                        height="80"
                    />
                </div>
            )}
        </div>
    );
};

export default Video;
