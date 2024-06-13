#define HTML_CSS_H

const char *HTML_CONTENT = R"=====(
     <div class="menu">
          <h1>ESP32 Control Panel</h1>
          <button id="scan">Scan</button>
          <button id="reset">Reset</button>
          <button id="info">Info</button>
          <button id="configure">Configure</button>
     </div>

     <div class=" number">
     </div>
     <div class="scanpopup">
          <div class="popup">
               <div class="close-popup">&times;</div>
               <div class="title">LIST WIFI</div>
               <table class="list-wifi" id="wifiTable">
                    <tr>
                         <th>
                              Name
                         </th>
                         <th>
                              RSSI
                         </th>
                    </tr>
               </table>
          </div>
     </div>
     <div class="loader-screen">
          <div class="loading-content">Scanning WIFI</div>
          <div class="loader"></div>
     </div>
     <div class="machine-status">
          <div class="status-off">OFF</div>
          <input type="checkbox" id="switch"><label for="switch">Toggle</label>
          <div class="status-on">ON</div>       
     </div>
     <div class="inputpw">
          <div class="popup-pw">
               <div class="name-wifi"></div>
               <div class="nhapmatkhau">Mật khẩu</div>
               <input id="passwordField" type="password" placeholder="Nhập mật khẩu">
               <div id="togglePassword" class="toggle-password">&#128065;</div>
               <div class="fncBtn">
                    <input type="button" id="connectButton" class="connect" value="Kết nối">
                    <input type="button" id="cancelButton" class="cancel" value="Hủy">
               </div>

          </div>
     </div>
     <div class="datetime-display">
          <span id="day-and-date"></span>
          <span id="time"></span>
     </div>
     <script>
          const scanButton = document.getElementById("scan");
          const scanPopup = document.querySelector(".scanpopup");
          const closePopup = document.querySelector(".close-popup");
          const loader = document.querySelector(".loader-screen");
          const listWifiTable = document.querySelector(".list-wifi");
          const wifiTable = document.getElementById("wifiTable");
          const hidepw = document.getElementById("togglePassword");
          const cancelpw = document.getElementById("cancelButton");
          const connectpw = document.getElementById("connectButton");
          const passwordField = document.getElementById('passwordField');
          const inputpw = document.querySelector('.inputpw');
          const nameWifi = document.querySelector('.name-wifi');
          const checkStatus = document.getElementById('switch');
          const resetButton = document.getElementById("reset");
          checkStatus.disabled= true;
          var connectwifi = "not_connect";
          scanButton.addEventListener("click", () => {
               loader.classList.add("showloader")
               fetch('/scan')
                    .then(response => response.json())
                    .then(data => {
                        //  console.log(data)
                         const objectData = Object.values(data);
                         while (wifiTable.rows.length > 1) {
                              wifiTable.deleteRow(1);
                         }
                         objectData.forEach(wifi => {
                              for (let i = 0; i < wifi.length; i++) {
                                   const name = wifi[i].name;
                                   const rssi = wifi[i].rssi;
                                   const row = document.createElement('tr');
                                   const nameCell = document.createElement('td');
                                   const rssiCell = document.createElement('td');
                                   nameCell.textContent = name;
                                   rssiCell.textContent = rssi;
                                   row.appendChild(nameCell);
                                   row.appendChild(rssiCell);
                                   wifiTable.appendChild(row);
                                   row.addEventListener("click", () => {
                                        inputpw.classList.add("showinputpw");
                                        nameWifi.innerHTML = name;
                                   });
                              }
                         })
                         loader.classList.remove("showloader");
                         scanPopup.classList.add("showscan");
                    })
                    .catch(error => {
                         console.error('Error:', error);
                         loader.classList.remove("showloader");
                    });
          });

          closePopup.addEventListener("click", () => {
               scanPopup.classList.remove("showscan");
          });
          hidepw.addEventListener('click', function () {
               if (passwordField.type == "password") {
                    passwordField.type = "text"
                    this.innerHTML = '&#128064;'
               }
               else if (passwordField.type == "text") {
                    passwordField.type = "password"
                    this.innerHTML = '&#128065;'
               }
          });
          cancelpw.addEventListener('click', () => {
               passwordField.value = '';
               inputpw.classList.remove('showinputpw');
          });
          connectpw.addEventListener('click', () => {
               let name = nameWifi.innerHTML;
               let pass = passwordField.value;
               const data = { name: name, pass: pass };
               loader.classList.add("showloader");
               // Tạo yêu cầu POST
               fetch('/connect', {
                    method: 'POST',
                    headers: {
                         'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(data)
               })
                    .then(response => {
                         if (!response.ok) {
                              throw new Error('Network response was not ok');
                         }
                         return response.json();
                    })
                    .then(data => {
                         console.log(data);
                         if (data.connect_wifi === "true") {
                              loader.classList.remove("showloader");
                              inputpw.classList.remove('showinputpw');
                              scanPopup.classList.remove("showscan");
                              connectwifi = "connected";
                              console.log("Connected to WiFi, IP Address:", data.ipaddress);
                              alert("WiFi connected successfully! IP Address: " + data.ipaddress);
                         } else if(data.connect_wifi === "false"){
                              loader.classList.remove("showloader");
                              passwordField.innerHTML="";
                         }
                         else {
                              console.log("Failed to connect to WiFi");
                         }
                    })
                    .catch(error => {
                         console.error('Error:', error);
                    });
          })

          var hall_sensor;
          var defaultValue=0
          function fetchData() {
               fetch("/pzem")
                    .then(response => response.json())
                    .then(data => {
                         console.log(data);
                         hall_sensor_current = data.current
                         defaultValue = hall_sensor_current
                         machineStatus = data.machine_status
                         updateRandomNumber(hall_sensor_current)
                         updateMachineStatus(machineStatus)
                    })
                    .catch(error => {
                         console.error('Error fetching data:', error)
                         updateRandomNumber(defaultValue)
                    });
          }
          resetButton.addEventListener("click", () => {
               fetch('/reset')
                    .catch(error => {
                         console.error('There was a problem with the fetch operation:', error);
                    });
          })  
          setInterval(fetchData, 1000);
          function updateRandomNumber(data) {
               document.querySelector('.number').innerText = data;
          }
          function updateMachineStatus(machineStatus) {
               checkStatus.checked = machineStatus === "RUNNING";
          }
          function checkWifiStatus() {
               fetch("/check_wifi_status")
                    .then(response => response.json())
                    .then(data => {
                         if (data.wifiStatus) {
                        document.body.classList.add('border-red');
                        document.body.classList.remove('border-yellow');
                    } else {
                        document.body.classList.add('border-yellow');
                        document.body.classList.remove('border-red');
                    }
                    })
                    .catch(error => {
                         console.log("err",error)
                    })
          }
          setInterval(checkWifiStatus,500);
          function checkDateTime() {
               fetch("/getdatetime")
                    .then(response => response.json())
                    .then(data => {
                         console.log(data)
                           // Assuming the data format is { dataTime: '11/06/2024 23:50:46' }
               const dateTimeString = data.dataTime;

               // Split the date and time
               const [date, time] = dateTimeString.split(' ');

               // Update the HTML elements
               document.getElementById('day-and-date').innerText = date;
               document.getElementById('time').innerText = time;
                    })
          }
          setInterval(checkDateTime, 1000);
     </script>
     
)=====";

const char *CSS_CONTENT = R"=====(
body {
  font-family: Arial, sans-serif;
  text-align: center;
  margin-top: 50px;
  display: block;
  user-select: none; /* Ngăn chọn văn bản */
}

button {
  display: inline-block;
  padding: 15px 25px;
  font-size: 24px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: #fff;
  background-color: #4caf50;
  border: none;
  border-radius: 15px;
  box-shadow: 0 9px #999;
  margin: 10px;
}

button:hover {
  background-color: #3e8e41;
}

button:active {
  background-color: #3e8e41;
  box-shadow: 0 5px #666;
  transform: translateY(4px);
}

.number {
  margin-top: 50px; /* Set the display to inline-block to allow for a specific width and height */
  display: inline-block;
  width: 100px;
  height: 50px;
  text-align: center;
  line-height: 50px;
  background-color: #ddd;
  border: 5px solid #818181;
  border-radius: 50px;
  padding: 5px;
  font-size: 20px;
}

.scanpopup {
  display: none;
  position: fixed;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  background-color: #1d1d22d7;
  z-index: 2;
}

.showscan {
  display: block;
}
.popup {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 50%;
  height: 70%;
  background-color: rgb(75, 80, 178);
  border: solid 5px #898989;
  border-radius: 10px;
  /* z-index: 10; */
  text-align: center;
  transform: translate(-50%, -50%);
  overflow: auto;
  padding: 20px;
  box-sizing: border-box;
}
.show {
  display: block;
}

.close-popup {
  position: absolute;
  top: 10px;
  right: 10px;
  font-size: 24px;
  font-weight: bold;
  cursor: pointer;
  color: white;
}

.border-red {
  border: 10px solid rgb(0, 255, 136);
}
.border-yellow {
  border: 10px solid yellow;
}

.title {
  /* margin-top: 50px; */
  font-size: 28px;
  font-weight: bold;
  color: white;
  width: 100%;
  padding-top: 30px;
  width: 100%;
}

.list-wifi {
  font-family: Arial, sans-serif;

  padding: 10px;
  border: 1px solid white;
  border-radius: 5px;
  background-color: #f2f2f2;
  width: 100%;
  max-width: 100%;
  height: 50%;
  overflow: auto;
  box-sizing: border-box;
}

.list-wifi th,
.list-wifi td {
  padding: 10px;
  text-align: left;
  border-bottom: 3px solid #ddd;
}

.list-wifi th {
  background-color: #4caf50;
  border-radius: 10px;
  color: white;
}
.list-wifi tr {
  border: solid;
  border-radius: 10px;
  cursor: pointer;
}
.list-wifi tr:hover {
  background-color: #313131;
  color: white;
}
/* HTML: <div class="loader"></div> */
.loader-screen {
  display: none;
  opacity: 0;
  background-color: #616161b2;
  position: fixed; /* Sử dụng fixed để phủ toàn bộ màn hình */
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  width: 100vw; /* Chiều rộng toàn màn hình */
  height: 100vh; /* Chiều cao toàn màn hình */
  flex-direction: column; /* Canh giữa nội dung theo chiều dọc */
  justify-content: center; /* Canh giữa theo chiều dọc */
  align-items: center; /* Canh giữa theo chiều ngang */
  z-index: 4;
  transition: opacity 0.5s ease;
}
.loading-content {
  margin-bottom: 20px; /* Khoảng cách giữa nội dung và loader */
  padding: 10px;
  border: solid white 5px;
  border-radius: 10px;
  font-size: 30px;
  font-family: "Lucida Sans", "Lucida Sans Regular", "Lucida Grande",
    "Lucida Sans Unicode", Geneva, Verdana, sans-serif;
  color: white;
  background-color: #1d1d22c0;
}
.loader {
  border: 16px solid #f3f3f3;
  border-radius: 50%;
  border-top: 16px solid #3498db;
  width: 120px;
  height: 120px;
  animation: spin 2s linear infinite; /* Thêm animation cho loader */
}
/* @keyframes l2 {
  to {
    transform: rotate(1turn);
  }
} */
@keyframes spin {
  0% {
    transform: rotate(0deg);
  }
  100% {
    transform: rotate(360deg);
  }
}

.showloader {
  display: flex; /* Chuyển display sang flex để hiện phần tử */
  opacity: 1; /* Tăng opacity lên 1 */
}

input[type="checkbox"] {
  height: 0;
  width: 0;
  visibility: hidden;
}

label {
  cursor: pointer;
  text-indent: -9999px;
  width: 200px;
  height: 100px;
  background: grey;
  display: block;
  border-radius: 100px;
  position: relative;
  margin: 0px 20px 0px 20px;
  z-index: 1;
}

label:after {
  content: "";
  position: absolute;
  top: 5px;
  left: 5px;
  width: 90px;
  height: 90px;
  background: #fff;
  border-radius: 90px;
  transition: 0.3s;
}

input:checked + label {
  background: #bada55;
}

input:checked + label:after {
  left: calc(100% - 5px);
  transform: translateX(-100%);
}

label:active:after {
  width: 130px;
}

.machine-status {
  margin-top: 10px;
  display: flex;
  flex-direction: row;
  font-size: 24px;
  font-family: "Franklin Gothic Medium", "Arial Narrow", Arial, sans-serif;
  align-items: center;
  justify-content: center;
}
.status-on,
.status-off {
  padding: 15px 25px;
  font-size: 24px;
  cursor: pointer;
  text-align: center;
  text-decoration: none;
  outline: none;
  color: #fff;
  background-color: #4caf50;
  border: none;
  border-radius: 15px;
  box-shadow: 0 9px #999;
}
.status-off {
  background-color: #c74a24;
}
* {
  -webkit-tap-highlight-color: transparent;
}

/* Optional: Remove outline on focus, for better user experience on mobile */
div:focus {
  outline: none;
}
.popup::-webkit-scrollbar {
  width: 12px; /* Width of the vertical scrollbar */
  height: 12px; /* Height of the horizontal scrollbar */
}

.popup::-webkit-scrollbar-track {
  background: #f1f1f1; /* Background of the scrollbar track */
  border-radius: 10px;
}

.popup::-webkit-scrollbar-thumb {
  background: #888; /* Color of the scrollbar thumb */
  border-radius: 10px;
  border: 3px solid #f1f1f1; /* Creates padding around the thumb */
}

.popup::-webkit-scrollbar-thumb:hover {
  background: #555; /* Color of the scrollbar thumb on hover */
}

.inputpw {
  display: none;
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  background-color: rgba(65, 65, 65, 0.836);
  justify-items: center;
  z-index: 3;
}

.showinputpw {
  display: block;
}

.popup-pw {
  top: 50%;
  left: 50%;
  position: absolute;
  width: 50%;
  height: 30%;
  background-color: white;
  transform: translate(-50%, -50%);
  border-radius: 30px;
  display: flex;
  flex-direction: column;
}

.name-wifi {
  margin-top: 10px;
  font-size: 20px;
  display: flex;
  justify-content: center;
  font-family: "Trebuchet MS", "Lucida Sans Unicode", "Lucida Grande",
    "Lucida Sans", Arial, sans-serif;
}
.nhapmatkhau {
  padding: 5px;
  padding-left: 10px;
  margin-top: 10px;
  padding-bottom: 2px;
  font-family: "Trebuchet MS", "Lucida Sans Unicode", "Lucida Grande",
    "Lucida Sans", Arial, sans-serif;
  font-size: 15px;
  font-weight: 300;
}
.popup-pw input {
  margin: 10px;
  padding: 5px;
  border: 2px solid;
  border-radius: 5px;
  margin-bottom: 1px;
}
.toggle-password {
  display: flex;
  margin-right: 15px;
  justify-content: end;
  font-size: 20px;
  margin-top: -5px;
  cursor: pointer;
}
.fncBtn {
  padding: 0;
  display: flex;
  flex-direction: row;
  justify-content: space-around;
}
.fncBtn input {
  margin-top: 5px;
  padding: 5px 15px;
  border-radius: 15px;
  cursor: pointer;
}
.fncBtn .connect {
  background-color: rgb(127, 219, 168);
}
.fncBtn .cancel {
  background-color: rgb(214, 114, 56);
}
.datetime-display {
    text-align: center;
    border: 5px solid burlywood;
    font-family: math;
    padding: 10px;
    }

#time {
        font-weight: 400;
        display: block;
        font-size: 30px;
        margin: 0 0 5px;
        letter-spacing: 5px;
    }

)=====";