import 'package:firebase_database/firebase_database.dart';

class DatabaseController {
  FirebaseDatabase database = FirebaseDatabase.instance;

  void getData() async {
    const address =
        '/TEST/b0boEGdZJMYNdxMqi9tQ0UdEXMC3/UCL/OPS/107/EM/Live/overall';
    DatabaseReference dbRef = database.ref();
    final snapshot = await dbRef.child(address).get();
    if (snapshot.exists) {
      print(snapshot.value);
    } else {
      print('No data available');
    }
  }
}
