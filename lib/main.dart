import 'dart:async';
import 'package:flutter/material.dart';
import 'package:sensors_plus/sensors_plus.dart';

void main() {
  runApp(const MasinaApp());
}

class MasinaApp extends StatelessWidget {
  const MasinaApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Telemetrie Masina',
      theme: ThemeData(
        primarySwatch: Colors.green,
        useMaterial3: true,
      ),
      home: const SensorDashboard(),
    );
  }
}

class SensorDashboard extends StatefulWidget {
  const SensorDashboard({Key? key}) : super(key: key);

  @override
  State<SensorDashboard> createState() => _SensorDashboardState();
}

class _SensorDashboardState extends State<SensorDashboard> {
  List<double>? _userAccelerometerValues;
  List<double>? _gyroscopeValues;
  final _streamSubscriptions = <StreamSubscription<dynamic>>[];

  @override
  void initState() {
    super.initState();
    // franare sau accelerare
    _streamSubscriptions.add(
      userAccelerometerEventStream().listen((UserAccelerometerEvent event) {
        setState(() {
          _userAccelerometerValues = <double>[event.x, event.y, event.z];
        });
      }),
    );
    // viraje
    _streamSubscriptions.add(
      gyroscopeEventStream().listen((GyroscopeEvent event) {
        setState(() {
          _gyroscopeValues = <double>[event.x, event.y, event.z];
        });
      }),
    );
  }

  @override
  void dispose() {
    super.dispose();
    for (final subscription in _streamSubscriptions) {
      subscription.cancel();
    }
  }

  @override
  Widget build(BuildContext context) {
    final userAccel = _userAccelerometerValues?.map((double v) => v.toStringAsFixed(2)).toList();
    final gyro = _gyroscopeValues?.map((double v) => v.toStringAsFixed(2)).toList();

    return Scaffold(
      appBar: AppBar(
        title: const Text('Senzori Telefon - Backup OBD', style: TextStyle(color: Colors.white)),
        backgroundColor: Colors.green[700],
      ),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('Accelerometru (Frânare / Accelerare)', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18, color: Colors.blue)),
              const SizedBox(height: 10),
              Text('Axa X (Stânga-Dreapta): ${userAccel?[0] ?? "0.00"} m/s²'),
              Text('Axa Y (Față-Spate): ${userAccel?[1] ?? "0.00"} m/s²'),
              Text('Axa Z (Sus-Jos): ${userAccel?[2] ?? "0.00"} m/s²'),
              const Divider(height: 50, thickness: 2),
              const Text('Giroscop (Viraje strânse)', style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18, color: Colors.orange)),
              const SizedBox(height: 10),
              Text('Axa X: ${gyro?[0] ?? "0.00"} rad/s'),
              Text('Axa Y: ${gyro?[1] ?? "0.00"} rad/s'),
              Text('Axa Z: ${gyro?[2] ?? "0.00"} rad/s'),
            ],
          ),
        ),
      ),
    );
  }
}