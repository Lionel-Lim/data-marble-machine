import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

class Dashboard extends StatefulWidget {
  final UserCredential credential;
  const Dashboard({super.key, required this.credential});

  @override
  State<Dashboard> createState() => _DashboardState();
}

class _DashboardState extends State<Dashboard>
    with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  late Stream<QuerySnapshot> _docStream;
  late String _uid;

  FirebaseAuth auth = FirebaseAuth.instance;

  @override
  void initState() {
    super.initState();

    // _uid = auth.currentUser!.uid;
    _uid = "UOkcczZbWlCvWeV5haM8";

    _controller = AnimationController(
      vsync: this, // the SingleTickerProviderStateMixin
      duration: const Duration(seconds: 5),
    )..repeat();

    _docStream = FirebaseFirestore.instance
        .collection('device/$_uid/test/overall/live/')
        .orderBy("time", descending: true)
        .snapshots();

    // Listen to changes
    _docStream.listen((snapshot) {
      final docs = snapshot.docs;
      if (docs.isNotEmpty) {
        print("Data updated!");
        print("${docs.length}");
        print("${docs.first.data()}");
        // Assuming you want to listen to the first document changes, you can implement different logic if needed
        final data = docs.first.data() as Map<String, dynamic>;
        final newSpeed = data['power'] as int?;
        if (newSpeed != null) {
          // Then you can update the duration of your animation controller
          _controller.duration = Duration(seconds: newSpeed);

          // And finally, restart the animation
          _controller.repeat();
        }
      } else {
        debugPrint("No data");
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    // screen width
    final double width = MediaQuery.of(context).size.width;
    DateTime now = DateTime.now();
    DateFormat formatter = DateFormat('dd MMM');
    String formattedDate = formatter.format(now);
    // DatabaseController db = DatabaseController();
    // db.getStructure();
    // auth.signOut();

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
          children: const [
            ListTile(
              leading: Icon(Icons.settings),
              title: Text('Settings'),
            ),
          ],
        ),
      ),
      body: Center(
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
                  const Text(
                    "Live",
                    style: TextStyle(
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
            AnimatedBuilder(
              animation: _controller,
              builder: (context, child) {
                return Transform.rotate(
                  angle: _controller.value * 2.0 * 3.14159,
                  child: child,
                );
              },
              child: const Image(
                width: 150,
                image: AssetImage("asset/images/cog.png"),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
