import 'dart:async';
import 'package:flutter/material.dart';
import 'package:url_launcher/url_launcher.dart';

class CountdownScreen extends StatefulWidget {
  const CountdownScreen({super.key});

  @override
  _CountdownScreenState createState() => _CountdownScreenState();
}

class _CountdownScreenState extends State<CountdownScreen> {
  late DateTime _targetDateTime;
  Duration _timeUntilTarget = Duration.zero;
  late Timer _timer;
  bool timerDone = false;
  String exhibitionText =
      "This project explores innovative ways to visualise real-time energy consumption data. By transforming intangible kilowatts into physical marbles, understanding energy use is more accessible and intriguing.\n\n Featuring components designed for swift, laser-cut manufacturing, the device promises an affordable, interactive, and open-source approach to energy awareness.\n\n An associated app enables users to delve into more detailed data, creating a portal for energy consciousness. Initially tested within the Connected Environments Lab, the machine translates the electricity use of 3D printers and Screens into an auditory-visual experience.";

  void _showImageDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          content: InteractiveViewer(
            child: Stack(
              children: <Widget>[
                Image.asset(
                  'asset/images/deviceWithText.png',
                  fit: BoxFit.cover,
                ),
                // const Positioned(
                //   top: 10,
                //   left: 10,
                //   child: Text(
                //     "Left LED\nDaily Electricity Usage",
                //     style: TextStyle(
                //       backgroundColor: Colors
                //           .black54, // This ensures that the text is readable
                //       color: Colors.white,
                //     ),
                //   ),
                // ),
                // const Positioned(
                //   top: 10,
                //   left: 400,
                //   child: Text(
                //     "Right LED\nLive Electricity Usage",
                //     style: TextStyle(
                //       backgroundColor: Colors
                //           .black54, // This ensures that the text is readable
                //       color: Colors.white,
                //     ),
                //   ),
                // ),
                // const Positioned(
                //   bottom: 10,
                //   left: 10,
                //   child: Text(
                //     "Base Empty Space\nDaily Electricity Usage in £\n0.5£ = 1 Marble",
                //     style: TextStyle(
                //       backgroundColor: Colors
                //           .black54, // This ensures that the text is readable
                //       color: Colors.white,
                //     ),
                //   ),
                // ),
                // const Positioned(
                //   bottom: 180,
                //   left: 350,
                //   child: Text(
                //     "Hole to Empty Space",
                //     style: TextStyle(
                //       backgroundColor: Colors
                //           .black54, // This ensures that the text is readable
                //       color: Colors.white,
                //     ),
                //   ),
                // ),
                // const Positioned(
                //   bottom: 120,
                //   left: 10,
                //   child: Text(
                //     "One Running Marble = 0.1p",
                //     style: TextStyle(
                //       backgroundColor: Colors
                //           .black54, // This ensures that the text is readable
                //       color: Colors.white,
                //     ),
                //   ),
                // )
              ],
            ),
          ),
        );
      },
    );
  }

  @override
  void initState() {
    super.initState();

    _targetDateTime = DateTime(DateTime.now().year, 8, 23, 13, 0, 0);
    // _targetDateTime = DateTime(DateTime.now().year, 8, 22, 10, 0, 0);
    _timeUntilTarget = _targetDateTime.isBefore(DateTime.now())
        ? Duration.zero
        : _targetDateTime.difference(DateTime.now());

    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      setState(() {
        _timeUntilTarget = _targetDateTime.isBefore(DateTime.now())
            ? Duration.zero
            : _targetDateTime.difference(DateTime.now());

        if (_timeUntilTarget == Duration.zero) {
          timer.cancel();
          timerDone = true;
          // Navigator.pushReplacementNamed(context, '/login');
        }
      });
    });
  }

  @override
  void dispose() {
    _timer.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return SelectionArea(
      selectionControls: materialTextSelectionControls,
      child: Scaffold(
        body: Container(
          decoration: const BoxDecoration(
            image: DecorationImage(
              image: AssetImage('asset/images/background.png'),
              fit: BoxFit.cover,
              opacity: 1,
            ),
          ),
          child: Container(
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.8),
            ),
            child: SingleChildScrollView(
              child: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    ExpansionTile(
                      title: const Text(
                        "Visualising Energy Data: \n Utilising the Enchanting Spectacle of Marble Machines",
                        textAlign: TextAlign.center,
                        style: TextStyle(
                          fontSize: 20,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      subtitle: const Text(
                        "Click here for more information",
                        textAlign: TextAlign.center,
                      ),
                      children: [
                        ListTile(
                          title: Text(exhibitionText),
                        )
                      ],
                    ),
                    const SizedBox(height: 50),
                    Text(
                      'Time left: ${_timeUntilTarget.inHours}:${_timeUntilTarget.inMinutes.remainder(60).toString().padLeft(2, '0')}:${_timeUntilTarget.inSeconds.remainder(60).toString().padLeft(2, '0')}',
                      style: const TextStyle(fontSize: 24),
                    ),
                    timerDone
                        ? ElevatedButton(
                            onPressed: () {
                              Navigator.pushReplacementNamed(context, '/login');
                            },
                            child: const Text("Enter"),
                          )
                        : const SizedBox(height: 10),
                    const SizedBox(height: 20),
                    ElevatedButton(
                      onPressed: () {
                        _showImageDialog(context);
                      },
                      child: const Text('See how it works'),
                    ),
                    const SizedBox(height: 40),
                    ElevatedButton(
                      onPressed: () async {
                        // Replace with your LinkedIn URL
                        await launchURL('https://www.linkedin.com/in/dylim/');
                      },
                      child: const Text('LinkedIn'),
                    ),
                    const Text("https://www.linkedin.com/in/dylim/"),
                    const SizedBox(height: 10),
                    ElevatedButton(
                      onPressed: () async {
                        // Replace with your GitHub URL
                        await launchURL('https://github.com/Lionel-Lim');
                      },
                      child: const Text('GitHub'),
                    ),
                    const Text("https://github.com/Lionel-Lim"),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  Future<void> launchURL(String urlString) async {
    Uri url = Uri.parse(urlString);
    if (await canLaunchUrl(url)) {
      await launchUrl(url, mode: LaunchMode.platformDefault);
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not launch $urlString')),
      );
    }
  }
}
