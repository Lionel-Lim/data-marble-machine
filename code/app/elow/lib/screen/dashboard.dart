import 'dart:async';

import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:elow/screen/settingdialog.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:syncfusion_flutter_charts/charts.dart';
import 'package:syncfusion_flutter_gauges/gauges.dart';
import 'package:url_launcher/url_launcher.dart';

enum ReadingStatus { on, off, error }

class Dashboard extends StatefulWidget {
  final UserCredential credential;
  const Dashboard({super.key, required this.credential});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _chartData {
  _chartData(this.x, this.y);
  final DateTime x;
  final double y;
}

const List<Widget> menu = <Widget>[
  Text('Overview'),
  Text('Live'),
  Text('History'),
];

class _DashboardState extends State<Dashboard>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late Stream<QuerySnapshot> _docStream;
  late String _uid;
  late int maxLiveUse;
  double liveUse = 0;
  String status = "Off";
  late StreamSubscription stream;
  List<_chartData> liveHistory = [];
  List<_chartData> dailyHistory = [];
  bool isRefreshing = true;
  final List<bool> _selectedMenu = <bool>[true, false, false];
  int _selectedMenuIndex = 0;
  late Future<Map<String, dynamic>> preferenceDataFuture;
  Map<String, dynamic>? preferenceData;
  Map<String, dynamic>? newPreferenceData;

  FirebaseAuth auth = FirebaseAuth.instance;

  int getMaxDaily() {
    int maxPower = 100;
    FirebaseFirestore.instance
        .collection('device/$_uid/sensors/overall/live/')
        .orderBy("power", descending: true)
        .limit(1)
        .get()
        .then(
      (snapshot) {
        final docs = snapshot.docs;
        if (docs.isNotEmpty) {
          // Assuming you want to listen to the first document changes, you can implement different logic if needed
          final data = docs.first.data();
          debugPrint("power is $data");
          maxPower = data['power'] as int;
        } else {
          debugPrint("No data");
        }
      },
    );
    return maxPower;
  }

  void setStatus(ReadingStatus readingStatus) {
    setState(() {
      switch (readingStatus) {
        case ReadingStatus.on:
          status = "Live";
          break;
        case ReadingStatus.off:
          status = "Off";
          break;
        case ReadingStatus.error:
          status = "Error";
          break;
      }
    });
  }

  Widget graphTest(List<_chartData> data) {
    // debugPrint("Graph data is $data");
    return SfCartesianChart(
      primaryXAxis: DateTimeAxis(
        // intervalType: DateTimeIntervalType.days,
        // interval: 1,
        dateFormat: DateFormat.d().add_jm(),
        labelRotation: 45,
      ),
      series: <LineSeries<_chartData, DateTime>>[
        LineSeries<_chartData, DateTime>(
          dataSource: data, // Use the data passed to the function
          xValueMapper: (_chartData data, _) => data.x,
          yValueMapper: (_chartData data, _) => data.y,
          markerSettings: const MarkerSettings(
            isVisible: true,
          ),
        ),
      ],
    );
  }

  Future<List<_chartData>> getLiveHistory() async {
    if (isRefreshing == true) {
      liveHistory = await _getLiveHistory();
      return liveHistory;
    } else {
      return liveHistory;
    }
  }

  Future<List<_chartData>> _getLiveHistory() async {
    List<_chartData> liveHistory = [];
    await FirebaseFirestore.instance
        .collection("device/$_uid/sensors/overall/live/")
        .orderBy("time", descending: true)
        .limit(30)
        .get()
        .then((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        debugPrint("-------------------");
        for (var doc in docs) {
          final data = doc.data();
          Timestamp timestamp = data['time'];
          DateTime dateTime = timestamp.toDate();
          // debugPrint(data.toString());
          // debugPrint(dateTime.toString());
          liveHistory.add(_chartData(dateTime, data['power']));
        }
        debugPrint("-------------------");
      }
    });
    return liveHistory;
  }

  Future<List<_chartData>> getDailyHistory({int limit = 10}) async {
    if (isRefreshing == true) {
      dailyHistory = await _getDailyHistory(limit: limit);
      return dailyHistory;
    } else {
      return dailyHistory;
    }
  }

  Future<List<_chartData>> _getDailyHistory({int limit = 30}) async {
    List<_chartData> tempDailyHistory = [];
    DateTime startOfToday =
        DateTime(DateTime.now().year, DateTime.now().month, DateTime.now().day);
    await FirebaseFirestore.instance
        .collection("device/$_uid/sensors/overall/history/")
        .where("time", isGreaterThanOrEqualTo: startOfToday)
        .orderBy("time", descending: true)
        .limit(limit)
        .get()
        .then((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        debugPrint("---------Daily--------");
        for (var doc in docs) {
          final data = doc.data();
          Timestamp timestamp = data['time'];
          DateTime dateTime = timestamp.toDate();
          // debugPrint(data.toString());
          // debugPrint(dateTime.toString());
          tempDailyHistory.add(_chartData(dateTime, data['today']));
        }
        debugPrint("-------------------");
      }
    });
    return tempDailyHistory;
  }

  Future<Map<String, double>> getOverviewInfo() async {
    Map<String, double> overviewInfo = {};
    await FirebaseFirestore.instance
        .collection("device/$_uid/sensors/overall/history/")
        .orderBy("time", descending: true)
        .limit(1)
        .get()
        .then((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        debugPrint("---------overview--------");
        for (var doc in docs) {
          final data = doc.data();
          overviewInfo["today"] = data['today'];
          overviewInfo["yesterday"] = data['yesterday'];
          overviewInfo["total"] = data['total'];
        }
        debugPrint("-------------------");
      }
    });
    return overviewInfo;
  }

  Future<void> fetchPreferenceData() async {
    DocumentSnapshot doc =
        await FirebaseFirestore.instance.doc('device/$_uid').get();
    if (doc.exists) {
      final data = doc.data() as Map<String, dynamic>;
      setState(() {
        preferenceData = data['preference'] as Map<String, dynamic>;
      });
    } else {
      throw Exception('Document does not exist');
    }
  }

  Future<bool> _updatePreference() async {
    setState(() {});
    try {
      await FirebaseFirestore.instance.doc('device/$_uid').update({
        'preference': preferenceData,
      });
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error updating preference: $e')),
      );
      return false;
    }
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Preference updated successfully!')),
    );
    return true;
  }

  Future<void> _showSettingsDialog() async {
    return showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Settings'),
          content: SingleChildScrollView(
            child: SettingsDialogContent(
              initialPreferenceData: preferenceData!,
              onSave: (updatedData) {
                preferenceData = updatedData;
                _updatePreference(); // This method will use newPreferenceData to update Firestore
              },
            ),
          ),
          actions: <Widget>[
            TextButton(
              child: const Text('Cancel'),
              onPressed: () {
                Navigator.of(context).pop();
              },
            ),
            // TextButton(
            //   child: const Text('Save'),
            //   onPressed: () {
            //     // Here, you can get the updated data from the SettingsDialogContent
            //     Navigator.of(context).pop();
            //   },
            // ),
          ],
        );
      },
    );
  }

  double mapValue(double value, double fromLow, double fromHigh, double toLow,
      double toHigh) {
    return toLow +
        ((value - fromLow) / (fromHigh - fromLow) * (toHigh - toLow));
  }

  Future<void> launchURL(String urlString) async {
    Uri url = Uri.parse(urlString);
    if (await canLaunchUrl(url)) {
      await launchUrl(url);
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not launch $urlString')),
      );
    }
  }

  @override
  void initState() {
    super.initState();

    // _uid = auth.currentUser!.uid;
    _uid = "b0boEGdZJMYNdxMqi9tQ0UdEXMC3";
    // auth.currentUser!.updateDisplayName("Guest");

    fetchPreferenceData();

    maxLiveUse = getMaxDaily();

    _controller = AnimationController(
      vsync: this, // the SingleTickerProviderStateMixin
      duration: const Duration(seconds: 5),
    )..repeat();

    _docStream = FirebaseFirestore.instance
        .collection('device/$_uid/sensors/overall/live/')
        .orderBy("time", descending: true)
        .snapshots();

    // Listen to changes
    stream = _docStream.listen(
      (snapshot) {
        final docs = snapshot.docs;
        if (docs.isNotEmpty) {
          print("Data updated!");
          print("${docs.first.data()}");
          // Assuming you want to listen to the first document changes, you can implement different logic if needed
          final data = docs.first.data() as Map<String, dynamic>;
          final power = data['power'] as int;
          final timestamp = data['time'] as Timestamp;
          final dateTime = timestamp.toDate();
          final currentTime = DateTime.now();
          if (power > maxLiveUse) {
            maxLiveUse = power;
          }
          final newSpeed =
              mapValue(power.toDouble(), 0, maxLiveUse.toDouble(), 10, 1);
          setState(() {
            liveHistory.insert(0, _chartData(dateTime, newSpeed.toDouble()));

            // check currentTime - dateTime > 1 minute
            if (currentTime.difference(dateTime).inMinutes > 1) {
              setStatus(ReadingStatus.off);
              liveUse = 0;
            } else {
              setStatus(ReadingStatus.on);
              liveUse = power.toDouble();
              _controller.duration = Duration(seconds: newSpeed.toInt());
            }
          });
          debugPrint("maxLiveUse is $maxLiveUse");
          debugPrint("speed is $newSpeed");

          // And finally, restart the animation
          _controller.repeat();
        } else {
          debugPrint("No data");
        }
      },
    )
      ..onDone(() {
        debugPrint("Task Done");
        setStatus(ReadingStatus.off);
      })
      ..onError((error) {
        debugPrint("Error:");
        debugPrint(error.toString());
        setStatus(ReadingStatus.error);
      });
  }

  @override
  Widget build(BuildContext context) {
    final double width = MediaQuery.of(context).size.width;
    DateTime now = DateTime.now();
    DateFormat formatter = DateFormat('dd MMM');
    String formattedDate = formatter.format(now);

    if (preferenceData == null) {
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    } else {
      return Scaffold(
        appBar: AppBar(
          title: SizedBox(
            width: double.infinity,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const SizedBox(
                  height: 20,
                ),
                Row(
                  children: [
                    const Icon(
                      Icons.account_circle,
                      size: 30,
                    ),
                    Text(
                      "  ${widget.credential.user!.displayName}",
                      textAlign: TextAlign.left,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w400,
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
        endDrawer: Drawer(
          child: ListView(
            padding: EdgeInsets.zero,
            children: [
              ListTile(
                leading: const Icon(Icons.settings),
                title: const Text('Settings'),
                onTap: _showSettingsDialog,
              ),
              const Divider(),
              // add dialog that shows some text
              ListTile(
                leading: const Icon(Icons.info),
                title: const Text('About'),
                onTap: () {
                  showDialog(
                    context: context,
                    builder: (BuildContext context) {
                      return AlertDialog(
                        title: const Text('About'),
                        content: const SingleChildScrollView(
                          child: ListBody(
                            children: <Widget>[
                              Text(
                                "Visualising Energy Data: \n Utilising the Enchanting Spectacle of Marble Machines",
                                textAlign: TextAlign.center,
                                style: TextStyle(
                                  fontSize: 20,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              Text(
                                  'This project explores innovative ways to visualise real-time energy consumption data. By transforming intangible kilowatts into physical marbles, understanding energy use is more accessible and intriguing.'),
                              SizedBox(height: 20),
                              Text(
                                  'Featuring components designed for swift, laser-cut manufacturing, the device promises an affordable, interactive, and open-source approach to energy awareness.'),
                              SizedBox(height: 20),
                              Text(
                                  'An associated app enables users to delve into more detailed data, creating a portal for energy consciousness. Initially tested within the Connected Environments Lab, the machine translates the electricity use of 3D printers and Screens into an auditory-visual experience.'),
                            ],
                          ),
                        ),
                        actions: <Widget>[
                          TextButton(
                            child: const Text('Close'),
                            onPressed: () {
                              Navigator.of(context).pop();
                            },
                          ),
                        ],
                      );
                    },
                  );
                },
              ),
              ListTile(
                leading: const Icon(Icons.logout),
                title: const Text('Logout'),
                onTap: () async {
                  await auth.signOut();
                  Navigator.pushReplacementNamed(context, '/login');
                },
              ),
              ListTile(
                leading:
                    const ImageIcon(AssetImage("asset/images/linkedin-in.png")),
                title: const Text("LinkedIn"),
                onTap: () async {
                  await launchURL("https://www.linkedin.com/in/dylim/");
                },
              ),
              ListTile(
                leading: const ImageIcon(AssetImage("asset/images/github.png")),
                title: const Text('github'),
                onTap: () async {
                  await launchURL("https://github.com/Lionel-Lim");
                },
              )
            ],
          ),
        ),
        body: SingleChildScrollView(
          child: Center(
            child: Column(
              children: [
                const SizedBox(
                  height: 20,
                ),
                SizedBox(
                  width: width,
                  child: Row(
                    children: [
                      const SizedBox(
                        width: 20,
                      ),
                      Text(
                        status,
                        style: const TextStyle(
                          fontSize: 40,
                          fontWeight: FontWeight.w400,
                        ),
                      ),
                      const SizedBox(
                        width: 20,
                      ),
                      Text(
                        formattedDate,
                        style: const TextStyle(
                          fontSize: 30,
                          fontWeight: FontWeight.w100,
                          color: Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),
                status == "Live"
                    ? AnimatedBuilder(
                        animation: _controller,
                        builder: (context, child) {
                          return Transform.rotate(
                            angle: _controller.value * 2.0 * 3.14159,
                            child: child,
                          );
                        },
                        child: Image(
                          width: width / 3,
                          image: const AssetImage("asset/images/cog.png"),
                        ),
                      )
                    : Image(
                        width: width / 3,
                        image: const AssetImage("asset/images/cog.png"),
                      ),
                const SizedBox(
                  height: 20,
                ),
                SizedBox(
                  width: width / 3,
                  child: SfLinearGauge(
                    minimum: 0,
                    maximum: (((maxLiveUse + 99) / 100) * 100) + 1,
                    barPointers: [
                      LinearBarPointer(
                        value: liveUse.toDouble(),
                      ),
                    ],
                    axisTrackStyle: const LinearAxisTrackStyle(
                      gradient: LinearGradient(
                        colors: [
                          Colors.green,
                          Colors.yellow,
                          Colors.red,
                        ],
                      ),
                    ),
                    markerPointers: [
                      LinearWidgetPointer(
                        value: liveUse.toDouble(),
                        position: LinearElementPosition.outside,
                        child: Text("${liveUse.toDouble()}"),
                      )
                    ],
                  ),
                ),
                const SizedBox(
                  height: 20,
                ),
                SizedBox(
                  width: width,
                  height: 60,
                  child: Center(
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        // DropdownButton(items: const [], onChanged: ),
                        ToggleButtons(
                          direction: Axis.horizontal,
                          onPressed: (int index) {
                            setState(
                              () {
                                // The button that is tapped is set to true, and the others to false.
                                for (int i = 0; i < _selectedMenu.length; i++) {
                                  _selectedMenu[i] = i == index;
                                }
                                _selectedMenuIndex = _selectedMenu
                                    .indexWhere((element) => element == true);
                              },
                            );
                          },
                          borderRadius:
                              const BorderRadius.all(Radius.circular(8)),
                          constraints: const BoxConstraints(
                            minHeight: 40.0,
                            minWidth: 80.0,
                          ),
                          isSelected: _selectedMenu,
                          children: menu,
                        ),
                      ],
                    ),
                  ),
                ),
                _selectedMenuIndex == 0
                    ? FutureBuilder(
                        future: getOverviewInfo(),
                        builder: (context, snapshot) {
                          if (snapshot.connectionState ==
                              ConnectionState.done) {
                            if (snapshot.hasData) {
                              Map<String, double> data =
                                  snapshot.data as Map<String, double>;
                              return Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    "Today: ${data['today']} kwh    ${((data['today'] ?? 0) / (double.tryParse(preferenceData!["unitCost"]?.toString() ?? '1') ?? 1)).toStringAsFixed(3)} £",
                                    textAlign: TextAlign.left,
                                  ),
                                  Text(
                                    "Yesterday: ${data['yesterday']} kwh    ${((data['yesterday'] ?? 0) / (double.tryParse(preferenceData!["unitCost"]?.toString() ?? '1') ?? 1)).toStringAsFixed(3)} £",
                                    textAlign: TextAlign.left,
                                  ),
                                  Text(
                                    "Today: ${data['total']} kwh    ${((data['total'] ?? 0) / (double.tryParse(preferenceData!["unitCost"]?.toString() ?? '1') ?? 1)).toStringAsFixed(3)} £",
                                    textAlign: TextAlign.left,
                                  ),
                                ],
                              );
                            } else {
                              // Handle the case when there is no data
                              return const Text('No data available');
                            }
                          } else {
                            return const CircularProgressIndicator();
                          }
                        },
                      )
                    : FutureBuilder(
                        future: _selectedMenuIndex == 1
                            ? getLiveHistory()
                            : getDailyHistory(),
                        builder: (context, snapshot) {
                          // debugPrint("Snapshot is $snapshot");
                          if (snapshot.connectionState ==
                              ConnectionState.done) {
                            if (snapshot.hasData) {
                              List<_chartData> data =
                                  snapshot.data as List<_chartData>;
                              // isRefreshing = false;
                              return graphTest(
                                  data); // Pass the fetched data to graphTest()
                            } else {
                              // Handle the case when there is no data
                              return const Text('No data available');
                            }
                          } else {
                            return const CircularProgressIndicator();
                          }
                        },
                      ),
              ],
            ),
          ),
        ),
      );
    }
  }
}
