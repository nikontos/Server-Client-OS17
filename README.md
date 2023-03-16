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
    

# Στατιστικά :
**_Παρακάτω δίνονται κάποιες μετρήσεις για τον μέσο χρόνο αναμονής και εκτέλεσης των αιτημάτων._**

> Όταν ο client δεν είναι πολυνηματικός το μέγεθος της στοίβας στον server δεν επηρεάζει την ταχύτητα, κύριο ρόλο παίζει ο αριθμός των νημάτων απο πλευράς server. Ο ιδανικός αριθμός φάνηκε να είναι το 4.

>Σε πολυνηματικό client και με μικρό μέγεθος στοίβα η αυξηση των νημάτων του server είχε την τάση να κάνει τον χρόνος αναμονής  και εξυπηρετησης των αιτημάτων να μειώνεται.
    
  **Μέγεθος στοίβας : 10 **

![1clientStack10.jpg](https://bitbucket.org/repo/LykoM4/images/1795584394-1clientStack10.jpg)
![2clientstack10.jpg](https://bitbucket.org/repo/LykoM4/images/4012437770-2clientstack10.jpg)
    

**Μέγεθος στοίβας : 2 **
![2clientStack2.jpg](https://bitbucket.org/repo/LykoM4/images/3209629331-2clientStack2.jpg)

**Μέγεθος στοίβας : 1 **
![3clientStack1.jpg](https://bitbucket.org/repo/LykoM4/images/3716260710-3clientStack1.jpg)
![3clientStack1.jpg](https://bitbucket.org/repo/LykoM4/images/996525522-3clientStack1.jpg)



# Ονόματα μελών ομάδας 

**_Χρήστος Ζώνιος 2194_**

**_Νικόλαος-Μάριος Κόντος 2193_**
