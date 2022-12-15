# Πολυνηματικός διακομιστής 
- Δέχεται αιτήσεις απο έναν πελάτη. Κάθε αίτηση που δέχεται αποθηκεύεται σε μία στοίβα απο τέτοιες αιτήσεις. Όσο η στοίβα *δεν* είναι άδεια, πολλά νήματα διαβάζουν τις πληροφορίες της στοίβας και εκτελούν ταυτόχρονα τις αιτήσεις. Σε περίπτωση που αυτή η στοίβα γεμίσει ο διακομιστής περιμένει το σήμα που θα τον ειδοποιήσει ότι άδειασε κάποια θέση, και μπορεί πλέον να δεχτεί το επόμενο αίτημα.
Αν ο διακομιστής λάβει το σήμα *CTRL+Z* επιστρέφει κάποια στατιστικά για τον χρόνο διεκπαιρέωσης των αιτημάτων και σταματάει η λειτουργία του.




# Βασικές δομές που ορίστηκαν 
 *   Ένα struct ονόματος *Stack* που κρατάει πληροφορία για τον περιγραφέα αρχείου κάθε σύνδεσης και την ώρα σύνδεσης.

* Πίνακας *consumerTid[ ]* μεγέθους *THREADNO*, ανάλογου των νημάτων, που αποθηκεύει σε κάθε θέση τα αναγνωριστηκά τους.

* Ένας πίνακας *Stack connectionStack[ ]* μεγέθους *STACK_SIZE* που αντιπροσωπεύει την στοίβα.

* Γίνεται χρήση ενος *struct timeval* που ορίζει η βιβλιοθήκη *<sys/time>* και κρατάει πληροφορία για τον χρόνο που πέρασε απο κάποια χρονική στιγμή και μέτα.



# Συναρτήσεις 
###   void signal_func(int sig) 

**_Signal handler_** 

    1) Xειρίζεται ένα σήμα απο το πληκτρολόγιο
    2) Τυπώνει το σύνολο των αιτήσεων που έχουν εξυπηρετηθέι, και στην συνέχεια υπολογίζει και τυπώνει τον μέσο χρόνο αναμονής και εξυπηρέτησης ενός αιτήματος.
    3) Απελευθερώνει τους πόρους του συστήματος που καταλαμβάνουν τα νήματα, και στην συνέχεια τερματίζει την λειτουργία του διακομιστή.

### void *thread_func(void *arg)

**_Η συνάρτηση αυτή περνιέται σαν όρισμα στην *pthread_create* και είναι υπεύθυνη για τις κύριες λειτουργίες του διακομιστή._**

    1) Διαχείριση της στοίβας
    2) Αποθήκευση χρόνων
    3) Επεξεργασία αιτήματος
    4) Αμοιβαίος αποκλεισμός
