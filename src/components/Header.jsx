
function Header (props) {
    const batteryVoltage = Math.floor((props.batteryVoltage/15) * 100) + "%";

    return (
        <header className="md:flex">
        <div className="fixed bg-gray-200 box-border w-full md:w-4/5 px-4 z-30 top-0 shadow-md h-10 md:h-16">
            <div className="w-full h-full text-base mx-auto max-w-8xl grid grid-flow-row grid-cols-3 items-center justify-around">
            <div className=" flex flex-col text-center items-center">
                    <div className="hidden md:flex flex-row items-center text-gray-500">
                        <span className="pr-1 font-semibold">
                            Flight Computer
                        </span>
                        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" fill="currentColor" className="size-4">
                            <path fillRule="evenodd" d="M8.914 6.025a.75.75 0 0 1 1.06 0 3.5 3.5 0 0 1 0 4.95l-2 2a3.5 3.5 0 0 1-5.396-4.402.75.75 0 0 1 1.251.827 2 2 0 0 0 3.085 2.514l2-2a2 2 0 0 0 0-2.828.75.75 0 0 1 0-1.06Z" clipRule="evenodd" />
                            <path fillRule="evenodd" d="M7.086 9.975a.75.75 0 0 1-1.06 0 3.5 3.5 0 0 1 0-4.95l2-2a3.5 3.5 0 0 1 5.396 4.402.75.75 0 0 1-1.251-.827 2 2 0 0 0-3.085-2.514l-2 2a2 2 0 0 0 0 2.828.75.75 0 0 1 0 1.06Z" clipRule="evenodd" />
                        </svg>
                    </div>
                    { props.isConnected ? (
                        <p className="font-bold text-emerald-500 ">Connected</p>
                    ):( 
                        <p className="font-bold text-gray-400 ">Disconnected</p>
                    )}
                </div>
                <div className=" flex flex-col text-center items-center">
                    <div className="hidden md:flex flex-row items-center text-gray-500">
                        <span className="pr-1 font-semibold">
                            Base Station
                        </span>
                        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" fill="currentColor" className="size-4">
                            <path fillRule="evenodd" d="M8.914 6.025a.75.75 0 0 1 1.06 0 3.5 3.5 0 0 1 0 4.95l-2 2a3.5 3.5 0 0 1-5.396-4.402.75.75 0 0 1 1.251.827 2 2 0 0 0 3.085 2.514l2-2a2 2 0 0 0 0-2.828.75.75 0 0 1 0-1.06Z" clipRule="evenodd" />
                            <path fillRule="evenodd" d="M7.086 9.975a.75.75 0 0 1-1.06 0 3.5 3.5 0 0 1 0-4.95l2-2a3.5 3.5 0 0 1 5.396 4.402.75.75 0 0 1-1.251-.827 2 2 0 0 0-3.085-2.514l-2 2a2 2 0 0 0 0 2.828.75.75 0 0 1 0 1.06Z" clipRule="evenodd" />
                        </svg>
                    </div>
                    { props.isConnected ? (
                        <p className="font-bold text-emerald-500 ">Connected</p>
                    ):( 
                        <p className="font-bold text-gray-400 ">Disconnected</p>
                    )}
                </div>
                <div className="flex flex-col text-center items-center">
                    <span className="hidden md:block text-gray-500 font-semibold">
                        Battery 
                    </span>
                    <div className="flex flex-row items-center text-emerald-500">
                        <span className="font-bold pr-1">{batteryVoltage}</span>
                        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" fill="currentColor" className="size-4 -rotate-90">
                        <path fillRule="evenodd" d="M1 6.25A2.25 2.25 0 0 1 3.25 4h8.5A2.25 2.25 0 0 1 14 6.25v.085a1.5 1.5 0 0 1 1 1.415v.5a1.5 1.5 0 0 1-1 1.415v.085A2.25 2.25 0 0 1 11.75 12h-8.5A2.25 2.25 0 0 1 1 9.75v-3.5Zm2.25-.75a.75.75 0 0 0-.75.75v3.5c0 .414.336.75.75.75h8.5a.75.75 0 0 0 .75-.75v-3.5a.75.75 0 0 0-.75-.75h-8.5Z" clipRule="evenodd" />
                        <path d="M4.75 7a.75.75 0 0 0-.75.75v.5c0 .414.336.75.75.75h2a.75.75 0 0 0 .75-.75v-.5A.75.75 0 0 0 6.75 7h-2Z" />
                        </svg>
                    </div>
                </div>
            </div>
        </div>
        </header>
        
       
    
        
        
    );
}

export default Header;