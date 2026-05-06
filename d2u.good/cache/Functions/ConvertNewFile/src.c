int ConvertNewFile(char *ipInFN, char *ipOutFN, CFlag *ipFlag, const char *progname, // Line 1427
                   int (*Convert)(FILE*, FILE*, CFlag *, const char *) // Line 1428
#ifdef D2U_UNICODE
                 , int (*ConvertW)(FILE*, FILE*, CFlag *, const char *) // Line 1430
#endif
                  ) // Line 1432
{ // Line 1433
  int RetVal = 0; // Line 1434
  FILE *InF = NULL; // Line 1435
  FILE *TempF = NULL; // Line 1436
  char *TempPath; // Line 1437
  const char *errstr; // Line 1438
#ifdef D2U_UNIFILE
   struct _stat StatBuf; // Line 1440
   wchar_t pathw[D2U_MAX_PATH]; // Line 1441
#else
  struct stat StatBuf; // Line 1443
#endif
  struct utimbuf UTimeBuf; // Line 1445
#ifndef NO_CHMOD
  mode_t mask; // Line 1447
#endif
  char *TargetFN = NULL; // Line 1449
  int ResolveSymlinkResult = 0; // Line 1450

  ipFlag->status = 0 ; // Line 1452

  /* Test if output file is a symbolic link */ // Line 1454
  if (symbolic_link(ipOutFN) && !ipFlag->Follow) { // Line 1455
    ipFlag->status |= OUTPUTFILE_SYMLINK ; // Line 1456
    /* Not a failure, skipping input file according spec. (keep symbolic link unchanged) */ // Line 1457
    return -1; // Line 1458
  } // Line 1459

  /* Test if input file is a regular file or symbolic link */ // Line 1461
  if (regfile(ipInFN, 1, ipFlag, progname)) { // Line 1462
    ipFlag->status |= NO_REGFILE ; // Line 1463
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1464
    return -1; // Line 1465
  } // Line 1466

  /* Test if input file target is a regular file */ // Line 1468
  if (symbolic_link(ipInFN) && regfile_target(ipInFN, ipFlag,progname)) { // Line 1469
    ipFlag->status |= INPUT_TARGET_NO_REGFILE ; // Line 1470
    /* Not a failure, skipping non-regular input file according spec. */ // Line 1471
    return -1; // Line 1472
  } // Line 1473

  /* Test if output file target is a regular file */ // Line 1475
  if (symbolic_link(ipOutFN) && (ipFlag->Follow == SYMLINK_FOLLOW) && regfile_target(ipOutFN, ipFlag,progname)) { // Line 1476
    ipFlag->status |= OUTPUT_TARGET_NO_REGFILE ; // Line 1477
    /* Failure, input is regular, cannot produce output. */ // Line 1478
    if (!ipFlag->error) ipFlag->error = 1; // Line 1479
    return -1; // Line 1480
  } // Line 1481

  /* retrieve ipInFN file date stamp */ // Line 1483
#ifdef D2U_UNIFILE
  d2u_MultiByteToWideChar(CP_UTF8, 0, ipInFN, -1, pathw, D2U_MAX_PATH); // Line 1485
  if (_wstat(pathw, &StatBuf)) { // Line 1486
#else
  if (stat(ipInFN, &StatBuf)) { // Line 1488
#endif
    if (ipFlag->verbose) { // Line 1490
      ipFlag->error = errno; // Line 1491
      errstr = strerror(errno); // Line 1492
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, ipInFN); // Line 1493
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1494
    } // Line 1495
    return -1; // Line 1496
  } // Line 1497

  /* can open in file? */ // Line 1499
  InF=OpenInFile(ipInFN); // Line 1500
  if (InF == NULL) { // Line 1501
    if (ipFlag->verbose) { // Line 1502
      ipFlag->error = errno; // Line 1503
      errstr = strerror(errno); // Line 1504
      D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, ipInFN); // Line 1505
      D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1506
    } // Line 1507
    return -1; // Line 1508
  } // Line 1509

  /* If output file is a symbolic link, optional resolve the link and modify  */ // Line 1511
  /* the target, instead of removing the link and creating a new regular file */ // Line 1512
  TargetFN = ipOutFN; // Line 1513
  if (symbolic_link(ipOutFN) && !RetVal) { // Line 1514
    ResolveSymlinkResult = 0; /* indicates that TargetFN need not be freed */ // Line 1515
    if (ipFlag->Follow == SYMLINK_FOLLOW) { // Line 1516
      ResolveSymlinkResult = ResolveSymbolicLink(ipOutFN, &TargetFN, ipFlag, progname); // Line 1517
      if (ResolveSymlinkResult < 0) { // Line 1518
        if (ipFlag->verbose) { // Line 1519
          D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1520
          D2U_UTF8_FPRINTF(stderr, _("problems resolving symbolic link '%s'\n"), ipOutFN); // Line 1521
        } // Line 1522
        RetVal = -1; // Line 1523
      } // Line 1524
    } // Line 1525
  } // Line 1526
  /* The symbolic link's target could be on another file system. rename() used below // Line 1527
   * can't move files to another file system. We need to create the temp file on the // Line 1528
   * target file system. // Line 1529
   */ // Line 1530

  /* can open temp output file? */ // Line 1532
  if((TempF = MakeTempFileFrom(TargetFN, &TempPath))==NULL) { // Line 1533
    if (ipFlag->verbose) { // Line 1534
      if (errno) { // Line 1535
        ipFlag->error = errno; // Line 1536
        errstr = strerror(errno); // Line 1537
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1538
        D2U_ANSI_FPRINTF(stderr, _("Failed to open temporary output file: %s\n"), errstr); // Line 1539
      } else { // Line 1540
        /*  In case temp path was too long on Windows, errno is 0. */ // Line 1541
        if (!ipFlag->error) ipFlag->error = 1; // Line 1542
      } // Line 1543
    } // Line 1544
    RetVal = -1; // Line 1545
  } // Line 1546

#if DEBUG
  if (TempPath != NULL) { // Line 1549
    D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1550
    D2U_UTF8_FPRINTF(stderr, _("using %s as temporary file\n"), TempPath); // Line 1551
  } // Line 1552
#endif

  if (!RetVal) // Line 1555
    if (check_unicode(InF, TempF, ipFlag, ipInFN, progname)) // Line 1556
      RetVal = -1; // Line 1557

  /* conversion successful? */ // Line 1559
#ifdef D2U_UNICODE
  if ((ipFlag->bomtype == FILE_UTF16LE) || (ipFlag->bomtype == FILE_UTF16BE)) { // Line 1561
    if ((!RetVal) && (ConvertW(InF, TempF, ipFlag, progname))) // Line 1562
      RetVal = -1; // Line 1563
    if (ipFlag->status & UNICODE_CONVERSION_ERROR) { // Line 1564
      if (!ipFlag->error) ipFlag->error = 1; // Line 1565
      RetVal = -1; // Line 1566
    } // Line 1567
  } else { // Line 1568
    if ((!RetVal) && (Convert(InF, TempF, ipFlag, progname))) // Line 1569
      RetVal = -1; // Line 1570
  } // Line 1571
#else
  if ((!RetVal) && (Convert(InF, TempF, ipFlag, progname))) // Line 1573
    RetVal = -1; // Line 1574
#endif

   /* can close in file? */ // Line 1577
  if (d2u_fclose(InF, ipInFN, ipFlag, "r", progname) == EOF) // Line 1578
    RetVal = -1; // Line 1579

  /* can close output file? */ // Line 1581
  if (TempF) { // Line 1582
    if (d2u_fclose(TempF, TempPath, ipFlag, "w", progname) == EOF) // Line 1583
      RetVal = -1; // Line 1584
  } // Line 1585

#ifndef NO_CHMOD
  if (!RetVal) // Line 1588
  { // Line 1589
    if (ipFlag->NewFile == 0) { /* old-file mode */ // Line 1590
       RetVal = chmod (TempPath, StatBuf.st_mode); /* set original permissions */ // Line 1591
    } else { // Line 1592
       mask = umask(0); /* get process's umask */ // Line 1593
       umask(mask); /* set umask back to original */ // Line 1594
       RetVal = chmod(TempPath, StatBuf.st_mode & ~mask); /* set original permissions, minus umask */ // Line 1595
    } // Line 1596

    if (RetVal) { // Line 1598
       if (ipFlag->verbose) { // Line 1599
         ipFlag->error = errno; // Line 1600
         errstr = strerror(errno); // Line 1601
         D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1602
         D2U_UTF8_FPRINTF(stderr, _("Failed to change the permissions of temporary output file %s:"), TempPath); // Line 1603
         D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1604
       } // Line 1605
    } // Line 1606
  } // Line 1607
#endif

#ifndef NO_CHOWN
  if (!RetVal && (ipFlag->NewFile == 0)) { /* old-file mode */ // Line 1611
     /* Change owner and group of the temporary output file to the original file's uid and gid. */ // Line 1612
     /* Required when a different user (e.g. root) has write permission on the original file. */ // Line 1613
     /* Make sure that the original owner can still access the file. */ // Line 1614
     if (chown(TempPath, StatBuf.st_uid, StatBuf.st_gid)) { // Line 1615
        if (ipFlag->AllowChown) { // Line 1616
          if (ipFlag->verbose) { // Line 1617
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1618
            D2U_UTF8_FPRINTF(stderr, _("The user and/or group ownership of file %s is not preserved.\n"), ipOutFN); // Line 1619
          } // Line 1620
#ifndef NO_CHMOD
          /* Set read/write permissions same as in new file mode. */ // Line 1622
          mask = umask(0); /* get process's umask */ // Line 1623
          umask(mask); /* set umask back to original */ // Line 1624
          RetVal = chmod(TempPath, StatBuf.st_mode & ~mask); /* set original permissions, minus umask */ // Line 1625
          if (RetVal) { // Line 1626
             if (ipFlag->verbose) { // Line 1627
               ipFlag->error = errno; // Line 1628
               errstr = strerror(errno); // Line 1629
               D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1630
               D2U_UTF8_FPRINTF(stderr, _("Failed to change the permissions of temporary output file %s:"), TempPath); // Line 1631
               D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1632
             } // Line 1633
          } // Line 1634
#endif
        } else { // Line 1636
          if (ipFlag->verbose) { // Line 1637
            ipFlag->error = errno; // Line 1638
            errstr = strerror(errno); // Line 1639
            D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1640
            D2U_UTF8_FPRINTF(stderr, _("Failed to change the owner and group of temporary output file %s:"), TempPath); // Line 1641
            D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1642
          } // Line 1643
          RetVal = -1; // Line 1644
        } // Line 1645
     } // Line 1646
  } // Line 1647
#endif

  if ((!RetVal) && (ipFlag->KeepDate)) // Line 1650
  { // Line 1651
    UTimeBuf.actime = StatBuf.st_atime; // Line 1652
    UTimeBuf.modtime = StatBuf.st_mtime; // Line 1653
    /* can change output file time to in file time? */ // Line 1654
    if (utime(TempPath, &UTimeBuf) == -1) { // Line 1655
      if (ipFlag->verbose) { // Line 1656
        ipFlag->error = errno; // Line 1657
        errstr = strerror(errno); // Line 1658
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TempPath); // Line 1659
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1660
      } // Line 1661
      RetVal = -1; // Line 1662
    } // Line 1663
  } // Line 1664

  /* any error? cleanup the temp file */ // Line 1666
  if (RetVal && (TempPath != NULL)) { // Line 1667
    if (d2u_unlink(TempPath) && (errno != ENOENT)) { // Line 1668
      if (ipFlag->verbose) { // Line 1669
        ipFlag->error = errno; // Line 1670
        errstr = strerror(errno); // Line 1671
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TempPath); // Line 1672
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1673
      } // Line 1674
      RetVal = -1; // Line 1675
    } // Line 1676
  } // Line 1677

  /* can rename temporary file to output file? */ // Line 1679
  if (!RetVal) { // Line 1680
#ifdef NEED_REMOVE
    if (d2u_unlink(TargetFN) && (errno != ENOENT)) { // Line 1682
      if (ipFlag->verbose) { // Line 1683
        ipFlag->error = errno; // Line 1684
        errstr = strerror(errno); // Line 1685
        D2U_UTF8_FPRINTF(stderr, "%s: %s:", progname, TargetFN); // Line 1686
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1687
      } // Line 1688
      RetVal = -1; // Line 1689
    } // Line 1690
#endif

    if (d2u_rename(TempPath, TargetFN) != 0) { // Line 1693
      if (ipFlag->verbose) { // Line 1694
        ipFlag->error = errno; // Line 1695
        errstr = strerror(errno); // Line 1696
        D2U_UTF8_FPRINTF(stderr, "%s: ", progname); // Line 1697
        D2U_UTF8_FPRINTF(stderr, _("problems renaming '%s' to '%s':"), TempPath, TargetFN); // Line 1698
        D2U_ANSI_FPRINTF(stderr, " %s\n", errstr); // Line 1699
#ifdef S_ISLNK
        if (ResolveSymlinkResult > 0) // Line 1701
          D2U_UTF8_FPRINTF(stderr, _("          which is the target of symbolic link '%s'\n"), ipOutFN); // Line 1702
#endif
        D2U_UTF8_FPRINTF(stderr, _("          output file remains in '%s'\n"), TempPath); // Line 1704
      } // Line 1705
      RetVal = -1; // Line 1706
    } // Line 1707

    if (ResolveSymlinkResult > 0) // Line 1709
      free(TargetFN); // Line 1710
  } // Line 1711
  free(TempPath); // Line 1712
  return RetVal; // Line 1713
} // Line 1714
