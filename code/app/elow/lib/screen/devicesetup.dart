import 'package:flutter/material.dart';

class DeviceSetUp extends StatelessWidget {
  const DeviceSetUp({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Device Setup'),
      ),
      body: const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Icon(
              Icons.construction,
              size: 100.0,
              color: Colors.orange,
            ),
            SizedBox(height: 20.0),
            Text(
              'We currently only support the registered devices.',
              style: TextStyle(
                fontSize: 24.0,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(height: 10.0),
            Text(
              'We are working hard to bring this to you soon.',
              style: TextStyle(
                fontSize: 18.0,
              ),
            ),
          ],
        ),
      ),
    );
  }
}
