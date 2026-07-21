import { useState } from "react";
import Button from "./Button";

function OverridesDropdown({
  onSendCommand,
  drogueArmed = false,
  mainArmed = false,
  isConnected = true,
}) {
  const [isOpen, setIsOpen] = useState(false);
  
  // PWM configuration state
  const [pwmConfig, setPwmConfig] = useState({
    vcc: 14.8,
    drogue_v: 9.0,
    main_v: 10.0,
    drogue_time: 3000,
    main_time: 5000,
  });

  const confirmAndSend = (label, cmd) => {
    if (window.confirm(`Are you sure you want to ${label}?`)) {
      onSendCommand?.(cmd);
    }
  };

  return (
    <div className="w-full p-2 rounded-2xl border-2 border-gray-700 relative">
      <div
        className="text-xs uppercase -mt-5 w-fit bg-white px-1 z-10 cursor-pointer"
        onClick={() => setIsOpen((v) => !v)}
      >
        Overrides
      </div>
      {isOpen && (
        <div className="mt-2 space-y-2">
          {/* Drogue row mimicking Arm card */}
          <div className={`text-base w-full pt-1 uppercase items-center text-center grid grid-cols-2 ${drogueArmed ? "text-emerald-500" : "text-blue-800"}`}>
            <div className="border-r-2 border-gray-800">DROGUE · {drogueArmed ? "ARMED" : "SAFE"}</div>
            <div>
              <Button
                onClick={() =>
                  confirmAndSend(
                    drogueArmed ? "Disarm Drogue" : "Arm Drogue",
                    drogueArmed ? "drogue_disarm" : "drogue_arm"
                  )
                }
                disabled={!isConnected}
                className={`px-2 py-1 rounded-full shadow-md border-2 border-box font-bold w-16 h-8 text-xs uppercase ${
                  !drogueArmed
                    ? "bg-rose-600 hover:bg-rose-700 text-gray-100"
                    : "bg-emerald-500 hover:bg-emerald-600 text-gray-100"
                }`}
              >
                {drogueArmed ? "Disarm" : "Arm"}
              </Button>
            </div>
          </div>

          {/* Main row mimicking Arm card */}
          <div className={`text-base w-full pt-1 uppercase items-center text-center grid grid-cols-2 ${mainArmed ? "text-emerald-500" : "text-blue-800"}`}>
            <div className="border-r-2 border-gray-800">MAIN · {mainArmed ? "ARMED" : "SAFE"}</div>
            <div>
              <Button
                onClick={() =>
                  confirmAndSend(
                    mainArmed ? "Disarm Main" : "Arm Main",
                    mainArmed ? "main_disarm" : "main_arm"
                  )
                }
                disabled={!isConnected}
                className={`px-2 py-1 rounded-full shadow-md border-2 border-box font-bold w-16 h-8 text-xs uppercase ${
                  !mainArmed
                    ? "bg-rose-600 hover:bg-rose-700 text-gray-100"
                    : "bg-emerald-500 hover:bg-emerald-600 text-gray-100"
                }`}
              >
                {mainArmed ? "Disarm" : "Arm"}
              </Button>
            </div>
          </div>

          {/* PWM Configuration Section */}
          <div className="mt-4 p-3 rounded-lg border border-gray-300 bg-gray-50">
            <div className="text-xs font-semibold uppercase text-gray-700 mb-2">PWM Configuration</div>
            <div className="space-y-2">
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="text-xs text-gray-600">VCC (V)</label>
                  <input
                    type="number"
                    step="0.1"
                    value={pwmConfig.vcc}
                    onChange={(e) => setPwmConfig({ ...pwmConfig, vcc: parseFloat(e.target.value) })}
                    className="w-full px-2 py-1 text-xs border border-gray-300 rounded"
                  />
                </div>
                <div>
                  <label className="text-xs text-gray-600">Drogue V (V)</label>
                  <input
                    type="number"
                    step="0.1"
                    value={pwmConfig.drogue_v}
                    onChange={(e) => setPwmConfig({ ...pwmConfig, drogue_v: parseFloat(e.target.value) })}
                    className="w-full px-2 py-1 text-xs border border-gray-300 rounded"
                  />
                </div>
              </div>
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="text-xs text-gray-600">Main V (V)</label>
                  <input
                    type="number"
                    step="0.1"
                    value={pwmConfig.main_v}
                    onChange={(e) => setPwmConfig({ ...pwmConfig, main_v: parseFloat(e.target.value) })}
                    className="w-full px-2 py-1 text-xs border border-gray-300 rounded"
                  />
                </div>
                <div>
                  <label className="text-xs text-gray-600">Drogue Time (ms)</label>
                  <input
                    type="number"
                    step="100"
                    value={pwmConfig.drogue_time}
                    onChange={(e) => setPwmConfig({ ...pwmConfig, drogue_time: parseInt(e.target.value) })}
                    className="w-full px-2 py-1 text-xs border border-gray-300 rounded"
                  />
                </div>
              </div>
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="text-xs text-gray-600">Main Time (ms)</label>
                  <input
                    type="number"
                    step="100"
                    value={pwmConfig.main_time}
                    onChange={(e) => setPwmConfig({ ...pwmConfig, main_time: parseInt(e.target.value) })}
                    className="w-full px-2 py-1 text-xs border border-gray-300 rounded"
                  />
                </div>
                <div className="flex items-end">
                  <Button
                    onClick={() => {
                      const cmd = `SET_PWM:${JSON.stringify(pwmConfig)}`;
                      if (window.confirm(`Send PWM config?\n${cmd}`)) {
                        onSendCommand?.(cmd);
                      }
                    }}
                    disabled={!isConnected}
                    className="w-full px-2 py-1 rounded-md shadow-md border-2 font-bold text-xs uppercase bg-blue-600 hover:bg-blue-700 text-white"
                  >
                    SET PWM
                  </Button>
                </div>
              </div>
              <div className="grid grid-cols-2 gap-2 mt-2">
                <Button
                  onClick={() => onSendCommand?.("PWM_STATUS")}
                  disabled={!isConnected}
                  className="px-2 py-1 rounded-md shadow-md border-2 font-bold text-xs uppercase bg-green-600 hover:bg-green-700 text-white"
                >
                  PWM STATUS
                </Button>
                <Button
                  onClick={() => onSendCommand?.("HELP")}
                  disabled={!isConnected}
                  className="px-2 py-1 rounded-md shadow-md border-2 font-bold text-xs uppercase bg-gray-600 hover:bg-gray-700 text-white"
                >
                  HELP
                </Button>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default OverridesDropdown;
