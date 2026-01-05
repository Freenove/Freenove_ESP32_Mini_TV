const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Freenove ESP32</title>
  <style>
    body {
      font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
      background-color: #f0f2f5;
      margin: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      color: #333;
    }
    
    .container {
      background: white;
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.1);
      width: 90%;
      max-width: 400px;
      text-align: center;
    }

    h2 { color: #444; margin-bottom: 20px; }

    input[type=text] {
      width: 100%;
      padding: 12px 15px;
      margin: 10px 0;
      box-sizing: border-box;
      border: 2px solid #ddd;
      border-radius: 8px;
      font-size: 16px;
      transition: border-color 0.3s;
    }
    input[type=text]:focus {
      border-color: #4CAF50;
      outline: none;
    }

    .btn-send {
      background-color: #4CAF50;
      color: white;
      padding: 12px;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      width: 100%;
      font-size: 16px;
      font-weight: bold;
      transition: background 0.3s;
      margin-bottom: 20px;
    }
    .btn-send:hover { background-color: #45a049; }
    .btn-send:active { transform: scale(0.98); }

    .presets {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      justify-content: center;
      margin-top: 10px;
    }
    
    .btn-preset {
      background-color: #e7f3ff;
      color: #007bff;
      border: 1px solid #cce5ff;
      padding: 8px 12px;
      border-radius: 20px;
      cursor: pointer;
      font-size: 14px;
      transition: all 0.2s;
    }
    .btn-preset:hover {
      background-color: #007bff;
      color: white;
    }

    #toast {
      visibility: hidden;
      min-width: 200px;
      background-color: #333;
      color: #fff;
      text-align: center;
      border-radius: 4px;
      padding: 10px;
      position: fixed;
      z-index: 1;
      left: 50%;
      bottom: 30px;
      transform: translateX(-50%);
      font-size: 14px;
      opacity: 0;
      transition: opacity 0.5s, bottom 0.5s;
    }
    #toast.show {
      visibility: visible;
      opacity: 1;
      bottom: 50px;
    }
  </style>
</head>
<body>

  <div class="container">
    <h2>📡 Send message</h2>
    
    <input type="text" id="msgInput" placeholder="Input text..." onkeydown="if(event.keyCode==13) sendMsg()">
    <button class="btn-send" onclick="sendMsg()">Send</button>

    <div style="border-top: 1px solid #eee; margin: 15px 0;"></div>
    <p style="color:#888; font-size:14px;">Quick Send:</p>
    
    <div class="presets">
      <button class="btn-preset" onclick="fillAndSend('Hello World!')">Hello</button>
      <button class="btn-preset" onclick="fillAndSend('(^_^)')">(^_^)</button>
      <button class="btn-preset" onclick="fillAndSend('(T_T)')">(T_T)</button>
      <button class="btn-preset" onclick="fillAndSend('ʕ•ᴥ•ʔ')">ʕ•ᴥ•ʔ</button>
      <button class="btn-preset" onclick="fillAndSend('٩(◕‿◕｡)۶')">٩(◕‿◕｡)۶</button>
      <button class="btn-preset" onclick="fillAndSend('❤️')">❤️</button>
      <button class="btn-preset" onclick="fillAndSend('₍ᐢ..ᐢ₎ᐝ')">₍ᐢ..ᐢ₎ᐝ</button>
      <button class="btn-preset" onclick="fillAndSend('SOS!')">SOS!</button>
      <button class="btn-preset" onclick="fillAndSend('hhh!')">hhh!</button>
    </div>
  </div>

  <div id="toast">Sent successfully!</div>

  <script>
    function sendMsg(text) {
      var input = document.getElementById("msgInput");
      var msg = text ? text : input.value;

      if (!msg) {
        showToast("Content cannot be empty", true);
        return;
      }

      var params = "msg=" + encodeURIComponent(msg);

      fetch("/submit", {
        method: "POST",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded"
        },
        body: params
      })
      .then(response => {
        if (response.ok) {
          console.log("Sent successfully");
          showToast("✅ Success: " + msg);
          if (!text) input.value = "";
        } else {
          showToast("❌ Error", true);
        }
      })
      .catch(error => {
        console.error("Error:", error);
        showToast("⚠️ Connection error", true);
      });
    }

    function fillAndSend(text) {
      sendMsg(text);
    }

    function showToast(message, isError) {
      var x = document.getElementById("toast");
      x.innerText = message;
      x.className = "show";
      if(isError) x.style.backgroundColor = "#d32f2f";
      else x.style.backgroundColor = "#333";
      
      setTimeout(function(){ x.className = x.className.replace("show", ""); }, 3000);
    }
  </script>
</body>
</html>
)rawliteral";