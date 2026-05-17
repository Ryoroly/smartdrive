import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:sensors_plus/sensors_plus.dart';

const String ESP32_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const String ESP32_WRITE_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const String ESP32_READ_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a9";

void main() {
  runApp(const SmartDriveApp());
}

class SmartDriveApp extends StatelessWidget {
  const SmartDriveApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SmartDrive Monitor',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue, brightness: Brightness.dark),
        useMaterial3: true,
      ),
      home: const DashboardScreen(),
    );
  }
}

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({Key? key}) : super(key: key);

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  BluetoothDevice? _espDevice;
  BluetoothCharacteristic? _writeCharacteristic;
  bool _isScanning = false;
  bool _isConnected = false;

  List<double> _userAccelerometerValues = [0.0, 0.0, 0.0];
  StreamSubscription<UserAccelerometerEvent>? _accelSubscription;
  Timer? _sendTimer;
  String _lastSentPayload = "Niciunul";

  double _aggressivenessScore = 0.0;

  @override
  void initState() {
    super.initState();
    _initBluetooth();
    _startSensors();
  }

  @override
  void dispose() {
    _accelSubscription?.cancel();
    _sendTimer?.cancel();
    if (_espDevice != null) _espDevice!.disconnect();
    super.dispose();
  }

  void _startSensors() {
    _accelSubscription = userAccelerometerEventStream().listen((UserAccelerometerEvent event) {
      setState(() {
        _userAccelerometerValues = [event.x, event.y, event.z];
      });
    });

    _sendTimer = Timer.periodic(const Duration(milliseconds: 100), (timer) {
      _sendDataToESP();
    });
  }

  void _initBluetooth() async {
    if (await FlutterBluePlus.isSupported == false) return;
    FlutterBluePlus.adapterState.listen((BluetoothAdapterState state) {
      if (state == BluetoothAdapterState.on) _scanForESP32();
    });
  }

  void _scanForESP32() async {
    setState(() => _isScanning = true);
    await FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

    FlutterBluePlus.scanResults.listen((results) async {
      for (ScanResult r in results) {
        if (r.device.advName == "ESP32_Telemetrie" || r.device.platformName == "ESP32_Telemetrie") {
          await FlutterBluePlus.stopScan();
          _connectToDevice(r.device);
          break;
        }
      }
    });
  }

  void _connectToDevice(BluetoothDevice device) async {
    try {
      await device.connect();
      setState(() {
        _espDevice = device;
        _isConnected = true;
      });

      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        if (service.uuid.toString() == ESP32_SERVICE_UUID) {
          for (BluetoothCharacteristic characteristic in service.characteristics) {
            if (characteristic.uuid.toString() == ESP32_WRITE_UUID) {
              _writeCharacteristic = characteristic;
            }
            if (characteristic.uuid.toString() == ESP32_READ_UUID) {
              await characteristic.setNotifyValue(true);
              characteristic.lastValueStream.listen((value) {
                if (value.isNotEmpty) {
                  String scoreStr = String.fromCharCodes(value);
                  setState(() {
                    _aggressivenessScore = double.tryParse(scoreStr) ?? _aggressivenessScore;
                  });
                }
              });
            }
          }
        }
      }
    } catch (e) {
      print("Eroare: $e");
    }
  }

  void _sendDataToESP() async {
    if (_isConnected && _writeCharacteristic != null) {
      String dataString = "${_userAccelerometerValues[0].toStringAsFixed(2)},"
                          "${_userAccelerometerValues[1].toStringAsFixed(2)},"
                          "${_userAccelerometerValues[2].toStringAsFixed(2)}";
      
      setState(() {
        _lastSentPayload = dataString;
      });
      await _writeCharacteristic!.write(dataString.codeUnits, withoutResponse: true);
    }
  }

  Map<String, dynamic> _calculeazaProfil(double scor) {
    if (scor <= 20) return {'tip': 'Foarte Bun (Calm)', 'mod_asig': '-5.0%', 'culoare': Colors.green, 'icon': Icons.eco};
    if (scor <= 40) return {'tip': 'Bun (Normal)', 'mod_asig': '0.0%', 'culoare': Colors.lightGreen, 'icon': Icons.sentiment_satisfied};
    if (scor <= 60) return {'tip': 'Moderat (Atentie)', 'mod_asig': '+${((scor - 40.0) / 60.0 * 80.0).toStringAsFixed(1)}%', 'culoare': Colors.orange, 'icon': Icons.warning};
    if (scor <= 80) return {'tip': 'Agresiv (Risc)', 'mod_asig': '+${((scor - 40.0) / 60.0 * 80.0).toStringAsFixed(1)}%', 'culoare': Colors.deepOrange, 'icon': Icons.speed};
    return {'tip': 'Foarte Agresiv', 'mod_asig': '+${((scor - 40.0) / 60.0 * 80.0).toStringAsFixed(1)}%', 'culoare': Colors.red, 'icon': Icons.local_fire_department};
  }

  @override
  Widget build(BuildContext context) {
    final profil = _calculeazaProfil(_aggressivenessScore);

    return Scaffold(
      appBar: AppBar(
        title: const Text('SmartDrive Dublu Monitor'),
        actions: [
          Icon(_isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled, 
               color: _isConnected ? Colors.blue : Colors.grey),
          const SizedBox(width: 15),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const Text("1. MONITOARE LOCALE (DATE TRIMISE LA ESP32)", 
                style: TextStyle(fontWeight: FontWeight.bold, color: Colors.blueAccent, letterSpacing: 1)),
            const SizedBox(height: 8),
            Card(
              color: Colors.blueGrey.withOpacity(0.1),
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text("Axa X (Stanga-Dreapta): ${_userAccelerometerValues[0].toStringAsFixed(2)} m/s²"),
                    Text("Axa Y (Franare-Acc.):  ${_userAccelerometerValues[1].toStringAsFixed(2)} m/s²"),
                    Text("Axa Z (Sus-Jos):       ${_userAccelerometerValues[2].toStringAsFixed(2)} m/s²"),
                    const Divider(color: Colors.grey),
                    Text("String trimis prin BLE:  ", style: TextStyle(color: Colors.grey[400])),
                    Text(_isConnected ? _lastSentPayload : "Deconectat (Nu se trimite)", 
                        style: const TextStyle(fontFamily: 'monospace', fontWeight: FontWeight.bold, color: Colors.greenAccent)),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 25),

            const Text("2. REZULTATE CALCULATE (PRIMIT DE LA ESP32)", 
                style: TextStyle(fontWeight: FontWeight.bold, color: Colors.orangeAccent, letterSpacing: 1)),
            const SizedBox(height: 8),
            
            Card(
              elevation: 4,
              child: Padding(
                padding: const EdgeInsets.all(20.0),
                child: Column(
                  children: [
                    const Text('SCOR AGRESIVITATE ESP32', style: TextStyle(fontSize: 12, color: Colors.grey)),
                    Text('${_aggressivenessScore.toInt()}%',
                        style: TextStyle(fontSize: 64, fontWeight: FontWeight.bold, color: profil['culoare'])),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(profil['icon'], color: profil['culoare'], size: 20),
                        const SizedBox(width: 8),
                        Text(profil['tip'], style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold, color: profil['culoare'])),
                      ],
                    ),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 12),
            
            Card(
              color: profil['culoare'].withOpacity(0.1),
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  children: [
                    const Text('RECALCULARE COST ASIGURARE (RCA)', style: TextStyle(fontSize: 12, color: Colors.white70)),
                    const SizedBox(height: 5),
                    Text(profil['mod_asig'], style: TextStyle(fontSize: 36, fontWeight: FontWeight.bold, color: profil['culoare'])),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 30),

            ElevatedButton.icon(
              icon: const Icon(Icons.analytics),
              label: const Text('Simuleaza raspuns ESP32 (Test UI)'),
              onPressed: () {
                setState(() {
                  _aggressivenessScore += 15;
                  if (_aggressivenessScore > 100) _aggressivenessScore = 0;
                });